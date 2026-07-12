#type compute
#version 450 core

// Half-resolution ground-truth ambient occlusion (Jimenez et al. style
// horizon-based slices) computed from the depth buffer alone (normals are
// reconstructed from depth). Output is applied to the HDR color during the
// tonemap pass with a multi-bounce-style remap.

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(binding = 0, rgba8) writeonly uniform image2D aoOut; // half-res AO (r channel)
layout(binding = 1) uniform sampler2D depthTex;             // full-res depth attachment

layout(std140, binding = 0) uniform GtaoParams {
	vec4 proj;   // x = P00, y = |P11|, z = P22, w = P32 (GL-convention projection)
	vec4 params; // x = radius (world), y = intensity, z = depthRange01 (1 on Metal), w unused
	vec4 dims;   // x = outW, y = outH, z = fullW, w = fullH
} u;

const float PI = 3.14159265359;
const int GTAO_DIRECTIONS = 2;
const int GTAO_STEPS = 6;

float linearViewDepth(float depthValue)
{
	// GL-style projection on both backends: Metal's depth is z_ndc directly,
	// GL's is (z_ndc+1)/2. viewZ = P32 / (z_ndc + P22), positive forward.
	float zNdc = (u.params.z > 0.5) ? depthValue : (2.0 * depthValue - 1.0);
	return u.proj.w / (zNdc + u.proj.z);
}

// View-space position of a full-res pixel (y is screen-down; the AO integral
// only depends on relative geometry, so the handedness doesn't matter).
vec3 viewPositionAt(ivec2 fullCoord)
{
	ivec2 fullDims = ivec2(u.dims.zw);
	fullCoord = clamp(fullCoord, ivec2(0), fullDims - 1);
	float depthValue = texelFetch(depthTex, fullCoord, 0).r;
	float viewZ = linearViewDepth(depthValue);
	vec2 ndc = (vec2(fullCoord) + 0.5) / vec2(fullDims) * 2.0 - 1.0;
	return vec3(ndc.x / u.proj.x * viewZ, ndc.y / u.proj.y * viewZ, viewZ);
}

// Interleaved gradient noise: cheap per-pixel rotation without a texture.
float ign(vec2 pixel)
{
	return fract(52.9829189 * fract(dot(pixel, vec2(0.06711056, 0.00583715))));
}

void main()
{
	ivec2 gid = ivec2(gl_GlobalInvocationID.xy);
	ivec2 outDims = ivec2(u.dims.xy);
	if (gid.x >= outDims.x || gid.y >= outDims.y)
		return;

	ivec2 fullCoord = gid * 2;
	ivec2 fullDims = ivec2(u.dims.zw);

	float depthValue = texelFetch(depthTex, clamp(fullCoord, ivec2(0), fullDims - 1), 0).r;
	if (depthValue >= 1.0) {
		imageStore(aoOut, gid, vec4(1.0)); // background
		return;
	}

	vec3 P = viewPositionAt(fullCoord);
	// Normal from depth via central differences (pick the smaller-delta side
	// to avoid smearing across depth edges).
	vec3 pR = viewPositionAt(fullCoord + ivec2(1, 0));
	vec3 pL = viewPositionAt(fullCoord - ivec2(1, 0));
	vec3 pD = viewPositionAt(fullCoord + ivec2(0, 1));
	vec3 pU = viewPositionAt(fullCoord - ivec2(0, 1));
	vec3 dx = (abs(pR.z - P.z) < abs(P.z - pL.z)) ? (pR - P) : (P - pL);
	vec3 dy = (abs(pD.z - P.z) < abs(P.z - pU.z)) ? (pD - P) : (P - pU);
	vec3 N = normalize(cross(dy, dx));
	vec3 V = normalize(-P);
	if (dot(N, V) < 0.0)
		N = -N;

	// Screen-space sampling radius (pixels at full res) for the world radius.
	float radiusPx = u.params.x * u.proj.y * u.dims.w * 0.5 / P.z;
	radiusPx = clamp(radiusPx, 2.0, 64.0);

	float noise = ign(vec2(gid));
	float occlusionSum = 0.0;

	for (int slice = 0; slice < GTAO_DIRECTIONS; ++slice) {
		float phi = (float(slice) + noise) * (PI / float(GTAO_DIRECTIONS));
		vec2 omega = vec2(cos(phi), sin(phi));

		// Project the normal into the slice plane.
		vec3 sliceDir = vec3(omega, 0.0);
		vec3 planeNormal = normalize(cross(sliceDir, V));
		vec3 tangent = cross(V, planeNormal);
		vec3 projectedN = N - planeNormal * dot(N, planeNormal);
		float projLen = length(projectedN);
		if (projLen < 1e-4) continue;
		projectedN /= projLen;

		float cosGamma = clamp(dot(projectedN, V), -1.0, 1.0);
		float gamma = acos(cosGamma) * sign(dot(projectedN, tangent));

		// Horizon search on both sides of the slice (side 0 = the negative
		// arc h1, side 1 = the positive arc h2).
		vec2 horizonCos = vec2(-1.0);
		for (int side = 0; side < 2; ++side) {
			float sideSign = (side == 0) ? -1.0 : 1.0;
			for (int s = 1; s <= GTAO_STEPS; ++s) {
				float t = (float(s) - 1.0 + noise) / float(GTAO_STEPS);
				float stepPx = max(1.0, t * t * radiusPx); // denser near the center
				ivec2 sampleCoord = fullCoord + ivec2(sideSign * omega * stepPx);
				vec3 S = viewPositionAt(sampleCoord);
				vec3 delta = S - P;
				float distSq = dot(delta, delta);
				if (distSq < 1e-8) continue;
				float dist = sqrt(distSq);
				float cosH = dot(delta / dist, V);
				// Fade samples beyond the world radius so distant geometry
				// doesn't occlude.
				float falloff = clamp(1.0 - dist / u.params.x, 0.0, 1.0);
				cosH = mix(-1.0, cosH, falloff);
				horizonCos[side] = max(horizonCos[side], cosH);
			}
		}

		// GTAO inner integral (per side), referenced to the projected normal.
		float h1 = gamma + max(-acos(clamp(horizonCos.x, -1.0, 1.0)) - gamma, -PI * 0.5);
		float h2 = gamma + min( acos(clamp(horizonCos.y, -1.0, 1.0)) - gamma,  PI * 0.5);
		float a1 = -cos(2.0 * h1 - gamma) + cosGamma + 2.0 * h1 * sin(gamma);
		float a2 = -cos(2.0 * h2 - gamma) + cosGamma + 2.0 * h2 * sin(gamma);
		occlusionSum += projLen * 0.25 * (a1 + a2);
	}

	float visibility = clamp(occlusionSum / float(GTAO_DIRECTIONS), 0.0, 1.0);
	// Intensity as a power curve on visibility.
	visibility = pow(visibility, max(u.params.y, 1e-3));

	imageStore(aoOut, gid, vec4(visibility, visibility, visibility, 1.0));
}
