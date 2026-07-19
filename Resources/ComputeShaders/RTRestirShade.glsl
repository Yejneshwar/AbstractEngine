#type compute
#version 460 core
#extension GL_EXT_ray_query : require

// Ray-traced pipeline, pass 4/4: ReSTIR spatial reuse + final shade + GI
// resolve + composite. Per pixel: merge a few validated neighbor reservoirs
// (spatial reuse), cast the final visibility ray on the winner, shade direct
// light, add radiance-cascade diffuse GI and prefiltered-environment
// specular, then composite against the raster image — raster-only geometry
// (lines, grid, sky) wins wherever it is closer than the traced surface.

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(binding = 0, rgba16f) writeonly uniform image2D colorOut;

layout(binding = 1) uniform sampler2D hitDataTex;
layout(binding = 2) uniform sampler2D normalDepthTex;
layout(binding = 3) uniform sampler2D reservoirTex;
layout(binding = 4) uniform sampler2D cascade0Tex;
layout(binding = 6) uniform sampler2D rasterColorTex;
layout(binding = 7) uniform sampler2D rasterDepthTex;

#include <Resource/ComputeShaders/RTCommon.h>

const uint kProbeSpacing0 = 8u;

// Diffuse irradiance at the pixel from cascade 0 (which, after the top-down
// merge, holds the fully integrated radiance per direction). Edge-aware
// bilinear over the 4 surrounding probes; cosine-weighted sum over each
// probe's 8 directions. Returns E(n)/pi (multiply by diffuse albedo).
vec3 rtResolveGI(ivec2 gid, RTSurface s, float viewDepth)
{
	ivec2 dims = ivec2(u.screen.xy);
	ivec2 probes = (dims + int(kProbeSpacing0) - 1) / int(kProbeSpacing0);
	ivec2 dirs = ivec2(4, 2);

	vec2 probeF = (vec2(gid) + 0.5) / float(kProbeSpacing0) - 0.5;
	ivec2 base = ivec2(floor(probeF));
	vec2 f = probeF - vec2(base);

	vec3 irradiance = vec3(0.0);
	float weightSum = 0.0;

	for (int py = 0; py < 2; py++) {
		for (int px = 0; px < 2; px++) {
			ivec2 probe = clamp(base + ivec2(px, py), ivec2(0), probes - 1);
			ivec2 anchor = clamp(ivec2((vec2(probe) + 0.5) * float(kProbeSpacing0)), ivec2(0), dims - 1);
			vec4 anchorHit = texelFetch(hitDataTex, anchor, 0);
			if (anchorHit.w < 0.0)
				continue;
			vec4 anchorND = texelFetch(normalDepthTex, anchor, 0);

			float w = (px == 0 ? 1.0 - f.x : f.x) * (py == 0 ? 1.0 - f.y : f.y);
			// Edge awareness: don't gather across depth or orientation breaks.
			w *= exp(-8.0 * abs(anchorND.w - viewDepth) / max(viewDepth, 1e-3));
			w *= pow(max(dot(anchorND.xyz, s.normal), 0.0), 4.0);
			if (w <= 1e-5)
				continue;

			vec3 cosSum = vec3(0.0);
			for (int dy = 0; dy < 2; dy++) {
				for (int dx = 0; dx < 2; dx++) {
					// 4x2 direction tile, unrolled two directions at a time.
					for (int half2 = 0; half2 < 2; half2++) {
						ivec2 dirTile = ivec2(dx * 2 + half2, dy);
						vec3 dir = rtOctDecode((vec2(dirTile) + 0.5) / vec2(dirs));
						float cosTerm = max(dot(dir, s.normal), 0.0);
						if (cosTerm <= 0.0)
							continue;
						ivec2 texel = ivec2(dirTile.x * probes.x + probe.x,
						                    dirTile.y * probes.y + probe.y);
						vec4 radiance = texelFetch(cascade0Tex, texel, 0);
						cosSum += radiance.rgb * radiance.a * cosTerm;
					}
				}
			}
			// Solid angle per direction = 4pi/8; E = sum(L*cos*dOmega);
			// dividing by pi for the Lambert term leaves a factor 1/2.
			irradiance += cosSum * 0.5 * w;
			weightSum += w;
		}
	}

	if (weightSum <= 1e-4)
		return rtIrradianceSH(s.normal); // isolated pixel: SH fallback
	return irradiance / weightSum;
}

void main()
{
	ivec2 gid = ivec2(gl_GlobalInvocationID.xy);
	ivec2 dims = ivec2(u.screen.xy);
	if (gid.x >= dims.x || gid.y >= dims.y)
		return;

	vec4 rasterColor = texelFetch(rasterColorTex, gid, 0);
	vec4 hit = texelFetch(hitDataTex, gid, 0);
	if (hit.w < 0.0) {
		imageStore(colorOut, gid, rasterColor); // nothing traced: sky/lines
		return;
	}

	float viewDepth = texelFetch(normalDepthTex, gid, 0).w;

	// Raster geometry that isn't in the acceleration structure (lines,
	// circles, grid) occludes the traced surface when it's closer.
	float rasterDepth = texelFetch(rasterDepthTex, gid, 0).r;
	if (rasterDepth < 1.0) {
		float rasterViewZ = rtLinearViewDepth(rasterDepth);
		if (rasterViewZ < viewDepth - max(2e-3 * viewDepth, 1e-4)) {
			imageStore(colorOut, gid, rasterColor);
			return;
		}
	}

	vec3 rayDir = rtPrimaryRayDir(vec2(gid));
	RTSurface s = rtFetchSurface(floatBitsToUint(hit.x), hit.yz, rayDir);
	vec3 V = -rayDir;

	uint rng = rtSeed(uvec2(gid), uint(u.camPos.w), 331u);

	// ---- Spatial reuse ----
	RTReservoir r = rtUnpackReservoir(texelFetch(reservoirTex, gid, 0));
	float selectedPdf = (r.s.key != RT_LIGHT_INVALID) ? rtTargetPdf(s, V, r.s) : 0.0;
	float wSum = selectedPdf * r.W * r.M;

	int taps = int(u.restir.y + 0.5);
	float radius = u.restir.z;
	for (int i = 0; i < taps; i++) {
		float angle = 2.399963 * float(i) + rtRand(rng) * 6.2831853;
		float dist = sqrt((float(i) + rtRand(rng)) / max(float(taps), 1.0)) * radius;
		ivec2 neighbor = gid + ivec2(round(vec2(cos(angle), sin(angle)) * dist));
		if (any(lessThan(neighbor, ivec2(0))) || any(greaterThanEqual(neighbor, dims)))
			continue;

		vec4 nND = texelFetch(normalDepthTex, neighbor, 0);
		if (nND.w <= 0.0
			|| abs(nND.w - viewDepth) > 0.1 * max(viewDepth, 1e-3)
			|| dot(nND.xyz, s.normal) < 0.9)
			continue;

		RTReservoir other = rtUnpackReservoir(texelFetch(reservoirTex, neighbor, 0));
		if (other.s.key == RT_LIGHT_INVALID || other.M <= 0.0 || other.W <= 0.0)
			continue;

		float pHat = rtTargetPdf(s, V, other.s);
		float weight = pHat * other.W * other.M;
		wSum += weight;
		float mergedM = r.M + other.M;
		if (weight > 0.0 && rtRand(rng) * wSum <= weight)
			r.s = other.s;
		r.M = mergedM;
	}
	selectedPdf = (r.s.key != RT_LIGHT_INVALID) ? rtTargetPdf(s, V, r.s) : 0.0;
	r.W = (selectedPdf > 0.0 && r.M > 0.0) ? wSum / (r.M * selectedPdf) : 0.0;

	// ---- Final visibility + direct light ----
	// Analytic key lights are shaded deterministically (noise-free); the
	// reservoir carries the many-light part (emissive/point/env).
	vec3 direct = rtAnalyticDirLights(s, V);
	if (r.W > 0.0) {
		vec3 shadowDir;
		float shadowDist;
		vec3 contribution = rtLightContribution(s, V, r.s, shadowDir, shadowDist);
		float eps = rtRayEpsilon(s.pos);
		vec3 origin = s.pos + s.geoNormal * eps;
		if (!rtOccluded(origin, shadowDir, eps, max(shadowDist - 2.0 * eps, eps))) {
			vec3 sampled = contribution * r.W;
			// Firefly clamp: a rare huge-W reservoir reads as a bright speck,
			// not as signal. Cap generously — near-field emissive pools are
			// legitimately bright (Le/d^2), and clamping too low visibly
			// flattens the glow around emitters.
			float lum = rtLuminance(sampled);
			if (lum > 100.0)
				sampled *= 100.0 / lum;
			direct += sampled;
		}
	}

	// ---- Ambient: radiance-cascade GI (diffuse) + prefiltered env (specular) ----
	vec3 diffuseAlbedo = s.albedo * (1.0 - s.metallic);
	vec3 giIrradiance = vec3(0.0);
	vec3 ambient = vec3(0.0);
	if (u.gi.z > 0.5) {
		giIrradiance = rtResolveGI(gid, s, viewDepth) * u.gi.y;
		ambient += diffuseAlbedo * giIrradiance;
	}
	// Specular: traced mirror reflections for smooth surfaces (deterministic
	// — no added noise), fading to the prefiltered environment as roughness
	// rises (a single mirror ray can't represent a wide GGX lobe, and there
	// is no denoiser to hide stochastic lobe samples).
	vec3 reflection = vec3(0.0);
	{
		float NoV = clamp(dot(s.normal, V), 1e-4, 1.0);
		vec3 f0 = mix(vec3(0.04), s.albedo, s.metallic);
		vec3 R = reflect(-V, s.normal);
		float mipCount = lighting.params0.w;
		int mip = int(s.roughness * max(mipCount - 1.0, 0.0) + 0.5);
		vec3 radianceR = (lighting.params0.z > 0.0) ? rtEnvRadiance(R, mip) : vec3(0.0);

		float tracedWeight = 1.0 - smoothstep(0.15, 0.45, s.roughness);
		// Below the facet horizon the mirror ray can only hit neighboring
		// facets of the same surface (terminator acne) — keep the env there.
		if (tracedWeight > 1e-3 && dot(R, s.geoNormal) > 0.0) {
			float eps = rtRayEpsilon(s.pos);
			uint rPrim;
			vec2 rBary;
			float rT;
			// Offset along the ray as well: grazing reflections on curved
			// tessellation otherwise re-hit the neighboring facets (black
			// speckles on smooth metals).
			if (rtTrace(s.pos + s.geoNormal * eps + R * eps, R, eps, 1e30, rPrim, rBary, rT)) {
				RTSurface hitSurface = rtFetchSurface(rPrim, rBary, R);
				radianceR = mix(radianceR, rtReflectionHitRadiance(hitSurface, -R), tracedWeight);
			}
			// Miss: the environment fetch above already IS the reflection.
		}
		reflection = radianceR * rtEnvBRDFApprox(f0, s.roughness, NoV);
		ambient += reflection;
	}

	vec3 color = direct + ambient + s.emissive;

	// ---- Debug views ----
	int debugView = int(u.misc.z + 0.5);
	if (debugView == 1)
		color = giIrradiance;
	else if (debugView == 2)
		color = direct;
	else if (debugView == 3)
		color = s.normal * 0.5 + 0.5;
	else if (debugView == 4)
		color = vec3(hit.w / (1.0 + hit.w));
	else if (debugView == 5)
		color = vec3(r.M / 64.0);
	else if (debugView == 6)
		color = reflection;

	imageStore(colorOut, gid, vec4(color, 1.0));
}
