#type compute
#version 460 core
#extension GL_EXT_ray_query : require

// Ray-traced pipeline, pass 2/4: ReSTIR DI — initial candidates + temporal
// reuse (Bitterli et al. 2020). Per pixel: RIS over a handful of light
// candidates (analytic lights, emissive triangles, environment), one shadow
// ray on the survivor (visibility reuse keeps occluded samples out of the
// history), then the reprojected previous-frame reservoir is merged in.

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(binding = 0, rgba32f) writeonly uniform image2D reservoirOut;

layout(binding = 1) uniform sampler2D hitDataTex;
layout(binding = 2) uniform sampler2D normalDepthTex;     // this frame
layout(binding = 3) uniform sampler2D prevNormalDepthTex; // last frame
layout(binding = 4) uniform sampler2D prevReservoirTex;

#include <Resource/ComputeShaders/RTCommon.h>

void main()
{
	ivec2 gid = ivec2(gl_GlobalInvocationID.xy);
	ivec2 dims = ivec2(u.screen.xy);
	if (gid.x >= dims.x || gid.y >= dims.y)
		return;

	vec4 hit = texelFetch(hitDataTex, gid, 0);
	if (hit.w < 0.0) {
		imageStore(reservoirOut, gid, rtPackReservoir(rtEmptyReservoir()));
		return;
	}

	vec3 rayDir = rtPrimaryRayDir(vec2(gid));
	RTSurface s = rtFetchSurface(floatBitsToUint(hit.x), hit.yz, rayDir);
	vec3 V = -rayDir;

	uint rng = rtSeed(uvec2(gid), uint(u.camPos.w), 17u);

	// ---- Initial candidates (RIS) ----
	RTReservoir r = rtEmptyReservoir();
	float wSum = 0.0;
	int candidates = int(u.restir.x + 0.5);
	for (int i = 0; i < candidates; i++) {
		float sourcePdf;
		RTLightSample cand = rtGenerateCandidate(s, rng, sourcePdf);
		if (cand.key == RT_LIGHT_INVALID) {
			r.M += 1.0;
			continue;
		}
		float pHat = rtTargetPdf(s, V, cand);
		rtUpdateReservoir(r, cand, pHat / max(sourcePdf, 1e-12), rng, wSum);
	}

	float selectedPdf = (r.s.key != RT_LIGHT_INVALID) ? rtTargetPdf(s, V, r.s) : 0.0;
	r.W = (selectedPdf > 0.0 && r.M > 0.0) ? wSum / (r.M * selectedPdf) : 0.0;

	// Visibility reuse: kill the survivor before it enters the history.
	if (r.W > 0.0) {
		vec3 shadowDir;
		float shadowDist;
		rtLightContribution(s, V, r.s, shadowDir, shadowDist);
		float eps = rtRayEpsilon(s.pos);
		vec3 origin = s.pos + s.geoNormal * eps;
		if (rtOccluded(origin, shadowDir, eps, max(shadowDist - 2.0 * eps, eps)))
			r.W = 0.0;
	}

	// ---- Temporal reuse ----
	if (u.screen.z > 0.5) {
		vec4 prevClip = u.prevViewProj * vec4(s.pos, 1.0);
		if (prevClip.w > 1e-6) {
			vec2 prevNdc = prevClip.xy / prevClip.w;
			ivec2 prevPixel = ivec2((prevNdc.x * 0.5 + 0.5) * u.screen.x,
			                        (0.5 - prevNdc.y * 0.5) * u.screen.y);
			if (all(greaterThanEqual(prevPixel, ivec2(0))) && all(lessThan(prevPixel, dims))) {
				vec4 prevND = texelFetch(prevNormalDepthTex, prevPixel, 0);
				float viewDepth = texelFetch(normalDepthTex, gid, 0).w;
				bool valid = prevND.w > 0.0
					&& abs(prevND.w - viewDepth) < 0.05 * max(viewDepth, 1e-3)
					&& dot(prevND.xyz, s.normal) > 0.9;
				if (valid) {
					RTReservoir prev = rtUnpackReservoir(texelFetch(prevReservoirTex, prevPixel, 0));
					if (prev.s.key != RT_LIGHT_INVALID && prev.M > 0.0) {
						// Cap the history so stale samples can't dominate.
						prev.M = min(prev.M, u.restir.w * max(r.M, 1.0));
						float pHatPrev = rtTargetPdf(s, V, prev.s);
						// Merge: the neighbor reservoir stands in for M
						// candidates with total weight pHat * W * M.
						float weight = pHatPrev * prev.W * prev.M;
						wSum += weight;
						float mergedM = r.M + prev.M;
						if (weight > 0.0 && rtRand(rng) * wSum <= weight)
							r.s = prev.s;
						r.M = mergedM;

						selectedPdf = (r.s.key != RT_LIGHT_INVALID) ? rtTargetPdf(s, V, r.s) : 0.0;
						r.W = (selectedPdf > 0.0 && r.M > 0.0) ? wSum / (r.M * selectedPdf) : 0.0;
					}
				}
			}
		}
	}

	imageStore(reservoirOut, gid, rtPackReservoir(r));
}
