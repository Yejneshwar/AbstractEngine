#type compute
#version 460 core
#extension GL_EXT_ray_query : require

// Ray-traced pipeline, pass 3/4: radiance cascades (Sannikov 2023) —
// screen-anchored probes with hardware-traced rays. Cascade i has probes
// every (8 << i) pixels, 8 * 4^i octahedral directions per probe, and traces
// the ray interval [r0*(4^i-1)/3, r0*(4^(i+1)-1)/3): probe density halves
// per axis while angular resolution quadruples, so near-field bounce light
// is spatially sharp and far-field light is angularly sharp.
//
// Dispatched top cascade first; a miss in cascade i continues into the
// (already-merged) cascade i+1, so cascade 0 ends up with the full
// integrated radiance per direction. One texel = one probe-direction:
// direction-major atlas, texel(dir.x * probesX + probe.x, dir.y * probesY + probe.y).

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(binding = 0, rgba16f) writeonly uniform image2D cascadeOut;

layout(binding = 1) uniform sampler2D upperCascadeTex;
layout(binding = 2) uniform sampler2D hitDataTex;
layout(binding = 3) uniform sampler2D normalDepthTex;

#include <Resource/ComputeShaders/RTCommon.h>

const uint kProbeSpacing0 = 8u;

// One-bounce radiance leaving the hit surface back along the probe ray.
// Emissive of the hit surface itself is EXCLUDED — direct light from
// emissive triangles is ReSTIR's job (counting it here would double it) —
// but the hit surface's own direct lighting (which includes emissive
// triangles as sources) is what bounces.
vec3 rtHitRadiance(RTSurface s, vec3 V, inout uint rng)
{
	vec3 diffuse = s.albedo * (1.0 - s.metallic) / RT_PI;
	vec3 radiance = vec3(0.0);

	// Directional lights: shadow ray on the first (key) light only.
	int dirCount = int(lighting.params0.x + 0.5);
	for (int i = 0; i < 4; i++) {
		if (i >= dirCount) break;
		vec3 L = -normalize(lighting.dirLights[i].direction.xyz);
		float NoL = dot(s.normal, L);
		if (NoL <= 0.0 || dot(s.geoNormal, L) <= 0.0) continue;
		if (i == 0) {
			float eps = rtRayEpsilon(s.pos);
			if (rtOccluded(s.pos + s.geoNormal * eps, L, eps, 1e30))
				continue;
		}
		radiance += diffuse * lighting.dirLights[i].color.rgb * NoL;
	}

	// Point lights: unshadowed (cheap approximation for bounce light).
	int pointCount = int(lighting.params0.y + 0.5);
	for (int i = 0; i < 8; i++) {
		if (i >= pointCount) break;
		vec3 toLight = lighting.pointLights[i].position.xyz - s.pos;
		float distSq = max(dot(toLight, toLight), 1e-8);
		vec3 L = toLight * inversesqrt(distSq);
		float NoL = dot(s.normal, L);
		if (NoL <= 0.0) continue;
		float attenuation = 1.0 / distSq;
		float radius = lighting.pointLights[i].position.w;
		if (radius > 0.0) {
			float sWin = clamp(1.0 - pow(distSq / (radius * radius), 2.0), 0.0, 1.0);
			attenuation *= sWin * sWin;
		}
		radiance += diffuse * lighting.pointLights[i].color.rgb * (attenuation * NoL);
	}

	// One unshadowed emissive-triangle sample so emissive light bounces
	// (LED glow on nearby surfaces). Deterministic pick — probes have no
	// temporal accumulation to hide per-frame noise.
	int emissiveCount = int(u.misc.x + 0.5);
	if (emissiveCount > 0) {
		int pick = min(int(rtRand(rng) * float(emissiveCount)), emissiveCount - 1);
		uint tri = rtEmissive[pick].triIndex;
		uint i0 = rtIndices[tri * 3u], i1 = rtIndices[tri * 3u + 1u], i2 = rtIndices[tri * 3u + 2u];
		vec3 p0 = rtPosition(i0), p1 = rtPosition(i1), p2 = rtPosition(i2);
		vec3 lightPos = (p0 + p1 + p2) / 3.0;
		vec3 toLight = lightPos - s.pos;
		float distSq = max(dot(toLight, toLight), 1e-8);
		vec3 L = toLight * inversesqrt(distSq);
		float NoL = dot(s.normal, L);
		vec3 lightNormal = normalize(cross(p1 - p0, p2 - p0));
		float sideSign = rtEmissive[pick].sideSign;
		float cosLight = (sideSign == 0.0)
			? abs(dot(lightNormal, L))
			: max(dot(lightNormal * sideSign, -L), 0.0);
		if (NoL > 0.0) {
			int materialIndex = floatBitsToInt(rtAttribs[i0].normalMat.w);
			vec3 Le = rtMaterials[materialIndex].emissive.rgb;
			// pdf of the uniform pick is 1/count; area-to-solid-angle with
			// the whole triangle standing in for the sample.
			float weight = float(emissiveCount) * rtEmissive[pick].area * cosLight / distSq;
			radiance += diffuse * Le * NoL * weight;
		}
	}

	return radiance;
}

void main()
{
	int cascade = int(u.misc.w + 0.5);
	uint spacing = kProbeSpacing0 << uint(cascade);
	ivec2 dims = ivec2(u.screen.xy);
	ivec2 probes = (dims + int(spacing) - 1) / int(spacing);
	ivec2 dirs = ivec2(4 << cascade, 2 << cascade);

	ivec2 gid = ivec2(gl_GlobalInvocationID.xy);
	if (gid.x >= probes.x * dirs.x || gid.y >= probes.y * dirs.y)
		return;

	ivec2 probe = ivec2(gid.x % probes.x, gid.y % probes.y);
	ivec2 dirTile = ivec2(gid.x / probes.x, gid.y / probes.y);

	// Probe anchor: the surface under the probe's pixel.
	ivec2 anchor = clamp(ivec2((vec2(probe) + 0.5) * float(spacing)), ivec2(0), dims - 1);
	vec4 hit = texelFetch(hitDataTex, anchor, 0);
	if (hit.w < 0.0) {
		imageStore(cascadeOut, gid, vec4(0.0)); // no surface: invalid probe
		return;
	}

	vec3 anchorRay = rtPrimaryRayDir(vec2(anchor));
	vec3 probePos = u.camPos.xyz + anchorRay * hit.w;
	vec3 probeNormal = texelFetch(normalDepthTex, anchor, 0).xyz;

	vec3 rayDir = rtOctDecode((vec2(dirTile) + 0.5) / vec2(dirs));

	// Interval hierarchy: r0 * [ (4^i - 1)/3, (4^(i+1) - 1)/3 ).
	float pow4 = exp2(2.0 * float(cascade));
	float t0 = u.gi.x * (pow4 - 1.0) / 3.0;
	float t1 = u.gi.x * (pow4 * 4.0 - 1.0) / 3.0;

	// Below-horizon directions can't contribute to the cosine-weighted
	// resolve; skip their rays.
	if (dot(rayDir, probeNormal) < -0.05) {
		imageStore(cascadeOut, gid, vec4(0.0, 0.0, 0.0, 1.0));
		return;
	}

	float eps = rtRayEpsilon(probePos);
	vec3 origin = probePos + probeNormal * eps;

	uint rng = rtSeed(uvec2(gid), uint(u.misc.w), 71u);

	uint prim;
	vec2 bary;
	float hitT;
	vec3 radiance;
	if (rtTrace(origin, rayDir, max(t0, eps), max(t1, eps * 2.0), prim, bary, hitT)) {
		RTSurface hitSurface = rtFetchSurface(prim, bary, rayDir);
		radiance = rtHitRadiance(hitSurface, -rayDir, rng);
	} else if (cascade == 4) { // top cascade: the miss escapes to the sky
		int mipCount = int(lighting.params0.w + 0.5);
		radiance = rtEnvRadiance(rayDir, max(mipCount - 1 - cascade, 0));
	} else {
		// Continue into the (already traced + merged) upper cascade:
		// bilinear over the 4 surrounding coarser probes, averaging the
		// 2x2 finer-direction block that subdivides this direction.
		uint upperSpacing = spacing * 2u;
		ivec2 upperProbes = (dims + int(upperSpacing) - 1) / int(upperSpacing);
		ivec2 upperDirs = dirs * 2;

		vec2 upperProbeF = (vec2(probe) + 0.5) * 0.5 - 0.5;
		ivec2 upperBase = ivec2(floor(upperProbeF));
		vec2 f = upperProbeF - vec2(upperBase);

		vec3 sum = vec3(0.0);
		float weightSum = 0.0;
		for (int py = 0; py < 2; py++) {
			for (int px = 0; px < 2; px++) {
				ivec2 upperProbe = clamp(upperBase + ivec2(px, py), ivec2(0), upperProbes - 1);
				float w = (px == 0 ? 1.0 - f.x : f.x) * (py == 0 ? 1.0 - f.y : f.y);
				vec3 dirSum = vec3(0.0);
				float dirValid = 0.0;
				for (int dy = 0; dy < 2; dy++) {
					for (int dx = 0; dx < 2; dx++) {
						ivec2 upperDir = dirTile * 2 + ivec2(dx, dy);
						ivec2 texel = ivec2(upperDir.x * upperProbes.x + upperProbe.x,
						                    upperDir.y * upperProbes.y + upperProbe.y);
						vec4 sampleValue = texelFetch(upperCascadeTex, texel, 0);
						dirSum += sampleValue.rgb * sampleValue.a;
						dirValid += sampleValue.a;
					}
				}
				if (dirValid > 0.0) {
					sum += (dirSum / dirValid) * w;
					weightSum += w;
				}
			}
		}
		if (weightSum > 1e-4) {
			radiance = sum / weightSum;
		} else {
			// No valid coarser probe (screen edge / isolated geometry):
			// fall back to the environment.
			int mipCount = int(lighting.params0.w + 0.5);
			radiance = rtEnvRadiance(rayDir, max(mipCount - 1 - cascade, 0));
		}
	}

	imageStore(cascadeOut, gid, vec4(radiance, 1.0));
}
