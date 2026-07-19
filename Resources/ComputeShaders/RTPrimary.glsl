#type compute
#version 460 core
#extension GL_EXT_ray_query : require

// Ray-traced pipeline, pass 1/4: primary visibility. One camera ray per
// pixel against the retained-mesh acceleration structure; writes the hit
// record (primitive + barycentrics + distance) and a world-normal/view-depth
// buffer used by ReSTIR validation and the radiance-cascade probes.

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(binding = 0, rgba32f) writeonly uniform image2D hitDataOut;    // primID bits, baryU, baryV, t (<0 = miss)
layout(binding = 1, rgba16f) writeonly uniform image2D normalDepthOut; // world normal, view depth

#include <Resource/ComputeShaders/RTCommon.h>

void main()
{
	ivec2 gid = ivec2(gl_GlobalInvocationID.xy);
	if (gid.x >= int(u.screen.x) || gid.y >= int(u.screen.y))
		return;

	vec3 origin = u.camPos.xyz;
	vec3 dir = rtPrimaryRayDir(vec2(gid));

	uint prim;
	vec2 bary;
	float hitT;
	if (!rtTrace(origin, dir, 1e-4, 1e30, prim, bary, hitT)) {
		imageStore(hitDataOut, gid, vec4(0.0, 0.0, 0.0, -1.0));
		imageStore(normalDepthOut, gid, vec4(0.0, 0.0, 0.0, 0.0));
		return;
	}

	RTSurface s = rtFetchSurface(prim, bary, dir);
	float viewDepth = -(u.view * vec4(s.pos, 1.0)).z; // positive forward

	imageStore(hitDataOut, gid, vec4(uintBitsToFloat(prim), bary.x, bary.y, hitT));
	imageStore(normalDepthOut, gid, vec4(s.normal, viewDepth));
}
