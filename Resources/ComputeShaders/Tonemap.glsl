#type compute
#version 450 core

// HDR -> LDR resolve for 3D viewports: applies the (half-res) GTAO with a
// multi-bounce-style remap, exposure, the chosen tonemap operator, and sRGB
// encoding, writing the display texture the UI shows.

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(binding = 0, rgba8) writeonly uniform image2D ldrOut;
layout(binding = 1, rgba16f) readonly uniform image2D hdrIn;
layout(binding = 2, rgba8) readonly uniform image2D aoTex; // half res

layout(std140, binding = 0) uniform TonemapParams {
	vec4 p0;   // x = exposure, y = operator (0 linear, 1 ACES, 2 AgX), z = aoEnabled, w = aoIntensity
	vec4 dims; // x = fullW, y = fullH, z = halfW, w = halfH
} u;

// Manual bilinear upsample of the half-res AO (image loads only — no
// sampler objects in the compute path).
float sampleAO(vec2 fullPixel)
{
	vec2 halfDims = u.dims.zw;
	vec2 p = fullPixel * 0.5 - 0.5;
	ivec2 p0 = ivec2(floor(p));
	vec2 f = p - vec2(p0);
	ivec2 maxCoord = ivec2(halfDims) - 1;

	float a00 = imageLoad(aoTex, clamp(p0 + ivec2(0, 0), ivec2(0), maxCoord)).r;
	float a10 = imageLoad(aoTex, clamp(p0 + ivec2(1, 0), ivec2(0), maxCoord)).r;
	float a01 = imageLoad(aoTex, clamp(p0 + ivec2(0, 1), ivec2(0), maxCoord)).r;
	float a11 = imageLoad(aoTex, clamp(p0 + ivec2(1, 1), ivec2(0), maxCoord)).r;

	return mix(mix(a00, a10, f.x), mix(a01, a11, f.x), f.y);
}

vec3 tonemapACES(vec3 x)
{
	// Narkowicz ACES filmic fit.
	const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
	return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Minimal AgX (log2 encoding + sigmoid contrast fit, no look transform).
vec3 agxContrast(vec3 x)
{
	vec3 x2 = x * x;
	vec3 x4 = x2 * x2;
	return +15.5 * x4 * x2
	       - 40.14 * x4 * x
	       + 31.96 * x4
	       - 6.868 * x2 * x
	       + 0.4298 * x2
	       + 0.1191 * x
	       - 0.00232;
}

vec3 tonemapAgX(vec3 color)
{
	const mat3 agxIn = mat3(
		0.842479062253094, 0.0423282422610123, 0.0423756549057051,
		0.0784335999999992, 0.878468636469772, 0.0784336,
		0.0792237451477643, 0.0791661274605434, 0.879142973793104);
	const mat3 agxOut = mat3(
		1.19687900512017, -0.0528968517574562, -0.0529716355144438,
		-0.0980208811401368, 1.15190312990417, -0.0980434501171241,
		-0.0990297440797205, -0.0989611768448433, 1.15107367264116);

	const float minEv = -12.47393;
	const float maxEv = 4.026069;

	color = agxIn * color;
	color = clamp(log2(max(color, vec3(1e-10))), minEv, maxEv);
	color = (color - minEv) / (maxEv - minEv);
	color = agxContrast(color);
	color = agxOut * color;
	return clamp(color, 0.0, 1.0);
}

vec3 linearToSrgb(vec3 c)
{
	c = clamp(c, 0.0, 1.0);
	vec3 lo = c * 12.92;
	vec3 hi = 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055;
	return mix(lo, hi, step(vec3(0.0031308), c));
}

void main()
{
	ivec2 gid = ivec2(gl_GlobalInvocationID.xy);
	if (gid.x >= int(u.dims.x) || gid.y >= int(u.dims.y))
		return;

	vec4 hdr = imageLoad(hdrIn, gid);
	vec3 color = hdr.rgb * u.p0.x;

	if (u.p0.z > 0.5) {
		float visibility = sampleAO(vec2(gid) + 0.5);
		// Jimenez-style multi-bounce remap: keeps some albedo-colored light
		// in occluded areas instead of crushing to black.
		float remapped = visibility * (0.5 + 0.5 * visibility);
		color *= mix(1.0, remapped, clamp(u.p0.w, 0.0, 1.0));
	}

	int op = int(u.p0.y + 0.5);
	if (op == 1)
		color = tonemapACES(color);
	else if (op == 2)
		color = tonemapAgX(color);
	// op == 0: linear (just clamped by the sRGB encode)

	imageStore(ldrOut, gid, vec4(linearToSrgb(color), 1.0));
}
