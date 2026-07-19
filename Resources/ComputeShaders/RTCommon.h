// Shared declarations for the ray-traced pipeline passes (RT*.glsl):
// ReSTIR DI + radiance-cascade GI. Compiled under the Vulkan SPIR-V target
// (GL_EXT_ray_query needs SPV_KHR_ray_query); at runtime Metal-only today,
// via SPIRV-Cross -> MSL 2.4 intersection queries.
//
// Conventions:
//  - All texture reads are texelFetch/textureSize (no sampler objects — the
//    compute backends never bind sampler state).
//  - Buffer bindings mirror RayTracing.cpp (params 0, lighting 2, scene 4-8,
//    acceleration structure 9).
//  - The parameter block mirrors RTParamsUBO in RayTracing.cpp; the lighting
//    block mirrors LightingUBOData in Lighting.h. Keep all three in sync.

// ---------------------------------------------------------------------------
// Parameter + lighting blocks
// ---------------------------------------------------------------------------

layout(std140, binding = 0) uniform RTParams {
	mat4 invView;
	mat4 view;
	mat4 prevViewProj;
	vec4 camPos;  // xyz = camera position, w = frame index
	vec4 proj;    // P00, P11, P22, P32 of the (ZO) projection
	vec4 screen;  // x = width, y = height, z = historyValid, w unused
	vec4 restir;  // x = initial candidates, y = spatial taps, z = spatial radius px, w = temporal M clamp
	vec4 gi;      // x = cascade0 interval end (world), y = GI intensity, z = GI enabled, w unused
	vec4 misc;    // x = emissive tri count, y unused, z = debug view, w = cascade index
} u;

struct DirLight { vec4 direction; vec4 color; };
struct PointLight { vec4 position; vec4 color; };

layout(std140, binding = 2) uniform LightingUBO {
	DirLight dirLights[4];
	PointLight pointLights[8];
	vec4 shIrradiance[9];
	vec4 params0; // x = dirCount, y = pointCount, z = envIntensity, w = envSpecularMipCount
	vec4 params1;
	vec4 params2;
} lighting;

// ---------------------------------------------------------------------------
// Scene buffers (MetalRayTracing) + acceleration structure
// ---------------------------------------------------------------------------

layout(std430, binding = 4) readonly buffer PositionsSSBO { float rtPositions[]; };

struct RTVertexAttrib {
	vec4 normalMat; // xyz = vertex normal, w = material index (int bits)
	vec4 color;     // per-vertex color (sRGB-authored, like the raster path)
};
layout(std430, binding = 5) readonly buffer AttribsSSBO { RTVertexAttrib rtAttribs[]; };

layout(std430, binding = 6) readonly buffer IndexSSBO { uint rtIndices[]; };

struct RTMaterial {
	vec4 baseColor; // sRGB-authored
	vec4 mrfx;      // x = metallic, y = roughness, z = flags (int)
	vec4 emissive;  // linear, premultiplied by intensity
};
layout(std430, binding = 7) readonly buffer MaterialSSBO { RTMaterial rtMaterials[]; };

struct RTEmissiveTri {
	uint triIndex;
	float area;
	// +1/-1: closed emitter, cross-product normal is outward/inward (emit
	// single-sided); 0: open surface, emit from both sides.
	float sideSign;
	float pad;
};
layout(std430, binding = 8) readonly buffer EmissiveSSBO { RTEmissiveTri rtEmissive[]; };

layout(binding = 9) uniform accelerationStructureEXT rtScene;

// Environment: GGX-prefiltered equirect (mip 0 = sharp). Bound in every pass.
layout(binding = 5) uniform sampler2D rtEnvTex;

#define RT_MATERIAL_FLAG_FLAT_NORMALS 1
#define RT_MATERIAL_FLAG_VERTEX_TINT 2

const float RT_PI = 3.14159265359;

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------

vec3 rtSrgbToLinear(vec3 c) { return pow(max(c, vec3(0.0)), vec3(2.2)); }
float rtLuminance(vec3 c) { return dot(c, vec3(0.2126, 0.7152, 0.0722)); }

// PCG hash RNG.
uint rtPcg(uint v)
{
	uint state = v * 747796405u + 2891336453u;
	uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	return (word >> 22u) ^ word;
}

float rtRand(inout uint state)
{
	state = rtPcg(state);
	return float(state) * (1.0 / 4294967296.0);
}

uint rtSeed(uvec2 pixel, uint frame, uint salt)
{
	return rtPcg(pixel.x * 1973u + pixel.y * 9277u + frame * 26699u + salt * 30011u);
}

// Full-sphere octahedral mapping (unit square <-> direction).
vec2 rtSignNotZero(vec2 v)
{
	return vec2(v.x >= 0.0 ? 1.0 : -1.0, v.y >= 0.0 ? 1.0 : -1.0);
}

vec2 rtOctEncode(vec3 d)
{
	d /= (abs(d.x) + abs(d.y) + abs(d.z));
	vec2 e = d.xy;
	if (d.z < 0.0)
		e = (1.0 - abs(d.yx)) * rtSignNotZero(d.xy);
	return e * 0.5 + 0.5;
}

vec3 rtOctDecode(vec2 e)
{
	e = e * 2.0 - 1.0;
	vec3 d = vec3(e.xy, 1.0 - abs(e.x) - abs(e.y));
	if (d.z < 0.0)
		d.xy = (1.0 - abs(d.yx)) * rtSignNotZero(d.xy);
	return normalize(d);
}

// Orthonormal basis around n (Duff et al.).
void rtBasis(vec3 n, out vec3 t, out vec3 b)
{
	float s = n.z >= 0.0 ? 1.0 : -1.0;
	float a = -1.0 / (s + n.z);
	t = vec3(1.0 + s * n.x * n.x * a, s * n.x * n.y * a, -s * n.x);
	b = vec3(n.x * n.y * a, s + n.y * n.y * a, -n.y);
}

vec2 rtDirToEquirect(vec3 d)
{
	float uCoord = atan(d.z, d.x) * 0.15915494309 + 0.5;
	float vCoord = acos(clamp(d.y, -1.0, 1.0)) * 0.31830988618;
	return vec2(uCoord, vCoord);
}

// Sampler-free environment lookup: manual bilinear inside the requested mip
// (the compute backends bind no sampler state, and the prefiltered mips are
// small enough that nearest-texel fetches band visibly on smooth surfaces).
vec3 rtEnvRadiance(vec3 dir, int lod)
{
	int maxLod = int(lighting.params0.w + 0.5) - 1;
	lod = clamp(lod, 0, max(maxLod, 0));
	ivec2 size = textureSize(rtEnvTex, lod);
	vec2 pos = rtDirToEquirect(dir) * vec2(size) - 0.5;
	ivec2 base = ivec2(floor(pos));
	vec2 f = pos - vec2(base);

	// U wraps (equirect seam), V clamps (poles).
	int x0 = (base.x % size.x + size.x) % size.x;
	int x1 = (x0 + 1) % size.x;
	int y0 = clamp(base.y, 0, size.y - 1);
	int y1 = clamp(base.y + 1, 0, size.y - 1);

	vec3 c00 = texelFetch(rtEnvTex, ivec2(x0, y0), lod).rgb;
	vec3 c10 = texelFetch(rtEnvTex, ivec2(x1, y0), lod).rgb;
	vec3 c01 = texelFetch(rtEnvTex, ivec2(x0, y1), lod).rgb;
	vec3 c11 = texelFetch(rtEnvTex, ivec2(x1, y1), lod).rgb;
	return mix(mix(c00, c10, f.x), mix(c01, c11, f.x), f.y) * lighting.params0.z;
}

// Diffuse irradiance/pi from the packed SH (same packing as the raster path).
vec3 rtIrradianceSH(vec3 n)
{
	return max(vec3(0.0),
		  lighting.shIrradiance[0].rgb
		+ lighting.shIrradiance[1].rgb * n.y
		+ lighting.shIrradiance[2].rgb * n.z
		+ lighting.shIrradiance[3].rgb * n.x
		+ lighting.shIrradiance[4].rgb * (n.x * n.y)
		+ lighting.shIrradiance[5].rgb * (n.y * n.z)
		+ lighting.shIrradiance[6].rgb * (3.0 * n.z * n.z - 1.0)
		+ lighting.shIrradiance[7].rgb * (n.x * n.z)
		+ lighting.shIrradiance[8].rgb * (n.x * n.x - n.y * n.y));
}

// ---------------------------------------------------------------------------
// Rays
// ---------------------------------------------------------------------------

// World-space primary ray through a pixel center. Texel row 0 is the top of
// the image; NDC +y is up.
vec3 rtPrimaryRayDir(vec2 pixel)
{
	vec2 ndc;
	ndc.x = 2.0 * (pixel.x + 0.5) / u.screen.x - 1.0;
	ndc.y = -(2.0 * (pixel.y + 0.5) / u.screen.y - 1.0);
	vec3 dirView = normalize(vec3(ndc.x / u.proj.x, ndc.y / u.proj.y, -1.0));
	return normalize((u.invView * vec4(dirView, 0.0)).xyz);
}

bool rtTrace(vec3 origin, vec3 dir, float tMin, float tMax,
             out uint primIndex, out vec2 bary, out float hitT)
{
	rayQueryEXT rq;
	rayQueryInitializeEXT(rq, rtScene, gl_RayFlagsOpaqueEXT, 0xFF, origin, tMin, dir, tMax);
	while (rayQueryProceedEXT(rq)) { }
	if (rayQueryGetIntersectionTypeEXT(rq, true) == gl_RayQueryCommittedIntersectionTriangleEXT) {
		primIndex = uint(rayQueryGetIntersectionPrimitiveIndexEXT(rq, true));
		bary = rayQueryGetIntersectionBarycentricsEXT(rq, true);
		hitT = rayQueryGetIntersectionTEXT(rq, true);
		return true;
	}
	primIndex = 0u;
	bary = vec2(0.0);
	hitT = -1.0;
	return false;
}

bool rtOccluded(vec3 origin, vec3 dir, float tMin, float tMax)
{
	rayQueryEXT rq;
	rayQueryInitializeEXT(rq, rtScene,
		gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT,
		0xFF, origin, tMin, dir, tMax);
	while (rayQueryProceedEXT(rq)) { }
	return rayQueryGetIntersectionTypeEXT(rq, true) != gl_RayQueryCommittedIntersectionNoneEXT;
}

// ---------------------------------------------------------------------------
// Surface reconstruction from a hit
// ---------------------------------------------------------------------------

struct RTSurface {
	vec3 pos;
	vec3 normal;    // shading normal, faces the incoming ray
	vec3 geoNormal; // facet normal, faces the incoming ray
	vec3 albedo;    // linear
	float metallic;
	float roughness;
	vec3 emissive;
	float alpha;
};

vec3 rtPosition(uint index)
{
	return vec3(rtPositions[index * 3u], rtPositions[index * 3u + 1u], rtPositions[index * 3u + 2u]);
}

RTSurface rtFetchSurface(uint prim, vec2 bary, vec3 rayDir)
{
	uint i0 = rtIndices[prim * 3u];
	uint i1 = rtIndices[prim * 3u + 1u];
	uint i2 = rtIndices[prim * 3u + 2u];
	vec3 p0 = rtPosition(i0), p1 = rtPosition(i1), p2 = rtPosition(i2);
	vec3 w = vec3(1.0 - bary.x - bary.y, bary.x, bary.y);

	RTSurface s;
	s.pos = w.x * p0 + w.y * p1 + w.z * p2;
	s.geoNormal = normalize(cross(p1 - p0, p2 - p0));

	RTVertexAttrib a0 = rtAttribs[i0], a1 = rtAttribs[i1], a2 = rtAttribs[i2];
	vec3 n = w.x * a0.normalMat.xyz + w.y * a1.normalMat.xyz + w.z * a2.normalMat.xyz;

	int materialIndex = floatBitsToInt(a0.normalMat.w);
	RTMaterial material = rtMaterials[materialIndex];
	int flags = int(material.mrfx.z + 0.5);

	if ((flags & RT_MATERIAL_FLAG_FLAT_NORMALS) != 0 || dot(n, n) < 0.25)
		n = s.geoNormal;
	else
		n = normalize(n);

	// CAD meshes are double-sided: shade the side the ray sees.
	if (dot(s.geoNormal, rayDir) > 0.0)
		s.geoNormal = -s.geoNormal;
	if (dot(n, rayDir) > 0.0)
		n = -n;
	s.normal = n;

	vec4 base = material.baseColor;
	if ((flags & RT_MATERIAL_FLAG_VERTEX_TINT) != 0)
		base *= w.x * a0.color + w.y * a1.color + w.z * a2.color;
	s.albedo = rtSrgbToLinear(base.rgb);
	s.alpha = base.a;
	s.metallic = clamp(material.mrfx.x, 0.0, 1.0);
	s.roughness = clamp(material.mrfx.y, 0.045, 1.0);
	s.emissive = material.emissive.rgb;
	return s;
}

// Scale-aware shadow/bounce ray offset. Sized to clear the sagitta of
// typical curved-surface tessellation (interpolated hit points sit slightly
// below the neighboring facets).
float rtRayEpsilon(vec3 pos)
{
	return max(1e-5, 5e-4 * length(pos - u.camPos.xyz));
}

// ---------------------------------------------------------------------------
// BRDF (identical terms to PBRMesh.glsl)
// ---------------------------------------------------------------------------

float rtD_GGX(float NoH, float a)
{
	float a2 = a * a;
	float d = (NoH * a2 - NoH) * NoH + 1.0;
	return a2 / (RT_PI * d * d);
}

float rtV_SmithGGXCorrelated(float NoV, float NoL, float a)
{
	float a2 = a * a;
	float gv = NoL * sqrt(NoV * NoV * (1.0 - a2) + a2);
	float gl = NoV * sqrt(NoL * NoL * (1.0 - a2) + a2);
	return 0.5 / max(gv + gl, 1e-5);
}

vec3 rtF_Schlick(vec3 f0, float VoH)
{
	float f = pow(1.0 - VoH, 5.0);
	return f0 + (vec3(1.0) - f0) * f;
}

vec3 rtEnvBRDFApprox(vec3 f0, float roughness, float NoV)
{
	const vec4 c0 = vec4(-1.0, -0.0275, -0.572, 0.022);
	const vec4 c1 = vec4(1.0, 0.0425, 1.04, -0.04);
	vec4 r = roughness * c0 + c1;
	float a004 = min(r.x * r.x, exp2(-9.28 * NoV)) * r.x + r.y;
	vec2 ab = vec2(-1.04, 1.04) * a004 + r.zw;
	return f0 * ab.x + ab.y;
}

// (diffuse + specular) * NoL for light direction L — multiply by incident
// radiance to get the reflected contribution.
vec3 rtBrdfCos(RTSurface s, vec3 V, vec3 L)
{
	float NoL = dot(s.normal, L);
	// The geometric-horizon test kills shadow-terminator acne on tessellated
	// surfaces: when the light is below the facet plane, the shadow ray can
	// only self-intersect the neighboring facets.
	if (NoL <= 0.0 || dot(s.geoNormal, L) <= 0.0)
		return vec3(0.0);
	float NoV = clamp(dot(s.normal, V), 1e-4, 1.0);
	float a = s.roughness * s.roughness;
	vec3 f0 = mix(vec3(0.04), s.albedo, s.metallic);

	vec3 H = normalize(V + L);
	float NoH = clamp(dot(s.normal, H), 0.0, 1.0);
	float VoH = clamp(dot(V, H), 0.0, 1.0);

	vec3 F = rtF_Schlick(f0, VoH);
	vec3 specular = rtD_GGX(NoH, a) * rtV_SmithGGXCorrelated(NoV, NoL, a) * F;
	vec3 diffuse = s.albedo * (1.0 - s.metallic) * (vec3(1.0) - F) / RT_PI;
	return (diffuse + specular) * NoL;
}

// Radiance leaving a reflection-ray hit back toward the reflecting surface:
// emissive (specular transport of emitters is NOT covered by ReSTIR DI, so
// it belongs here), diffuse direct light (shadow ray on the key light), and
// environment ambient — enough for convincing one-bounce reflections.
vec3 rtReflectionHitRadiance(RTSurface s, vec3 V)
{
	vec3 diffuse = s.albedo * (1.0 - s.metallic) / RT_PI;
	vec3 radiance = s.emissive;

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

	// Environment at the hit: SH diffuse + a mirror-in-mirror env specular.
	if (lighting.params0.z > 0.0) {
		radiance += s.albedo * (1.0 - s.metallic) * rtIrradianceSH(s.normal) * lighting.params0.z;
		float NoV = clamp(dot(s.normal, V), 1e-4, 1.0);
		vec3 f0 = mix(vec3(0.04), s.albedo, s.metallic);
		vec3 R = reflect(-V, s.normal);
		int mipCount = int(lighting.params0.w + 0.5);
		int mip = int(s.roughness * float(max(mipCount - 1, 0)) + 0.5);
		radiance += rtEnvRadiance(R, mip) * rtEnvBRDFApprox(f0, s.roughness, NoV);
	}

	return radiance;
}

// ---------------------------------------------------------------------------
// ReSTIR light samples + reservoirs
// ---------------------------------------------------------------------------

// key = kind << 28 | index; uv = triangle barycentrics (emissive) or
// octahedral direction (environment).
#define RT_LIGHT_DIR 0u
#define RT_LIGHT_POINT 1u
#define RT_LIGHT_EMISSIVE 2u
#define RT_LIGHT_ENV 3u
#define RT_LIGHT_INVALID 0xFFFFFFFFu

struct RTLightSample {
	uint key;
	vec2 uv;
};

uint rtSampleKind(RTLightSample ls) { return ls.key >> 28u; }
uint rtSampleIndex(RTLightSample ls) { return ls.key & 0x0FFFFFFFu; }

struct RTReservoir {
	RTLightSample s;
	float W; // unbiased contribution weight of the selected sample
	float M; // history length (candidate count)
};

RTReservoir rtEmptyReservoir()
{
	RTReservoir r;
	r.s.key = RT_LIGHT_INVALID;
	r.s.uv = vec2(0.0);
	r.W = 0.0;
	r.M = 0.0;
	return r;
}

vec4 rtPackReservoir(RTReservoir r)
{
	return vec4(uintBitsToFloat(r.s.key), uintBitsToFloat(packHalf2x16(r.s.uv)), r.W, r.M);
}

RTReservoir rtUnpackReservoir(vec4 packed4)
{
	RTReservoir r;
	r.s.key = floatBitsToUint(packed4.x);
	r.s.uv = unpackHalf2x16(floatBitsToUint(packed4.y));
	r.W = packed4.z;
	r.M = packed4.w;
	return r;
}

// Unshadowed contribution of a light sample at surface s (radiance already
// multiplied by BRDF*cos and geometry), plus the shadow-ray segment.
vec3 rtLightContribution(RTSurface s, vec3 V, RTLightSample ls, out vec3 shadowDir, out float shadowDist)
{
	uint kind = rtSampleKind(ls);
	uint index = rtSampleIndex(ls);
	shadowDir = vec3(0.0, 1.0, 0.0);
	shadowDist = 0.0;

	if (kind == RT_LIGHT_DIR) {
		vec3 L = -normalize(lighting.dirLights[index].direction.xyz);
		shadowDir = L;
		shadowDist = 1e30;
		return rtBrdfCos(s, V, L) * lighting.dirLights[index].color.rgb;
	}
	if (kind == RT_LIGHT_POINT) {
		vec3 toLight = lighting.pointLights[index].position.xyz - s.pos;
		float distSq = max(dot(toLight, toLight), 1e-8);
		float dist = sqrt(distSq);
		vec3 L = toLight / dist;
		float attenuation = 1.0 / distSq;
		float radius = lighting.pointLights[index].position.w;
		if (radius > 0.0) {
			float sWin = clamp(1.0 - pow(distSq / (radius * radius), 2.0), 0.0, 1.0);
			attenuation *= sWin * sWin;
		}
		shadowDir = L;
		shadowDist = dist;
		return rtBrdfCos(s, V, L) * lighting.pointLights[index].color.rgb * attenuation;
	}
	if (kind == RT_LIGHT_EMISSIVE) {
		if (index >= uint(u.misc.x + 0.5))
			return vec3(0.0);
		uint tri = rtEmissive[index].triIndex;
		uint i0 = rtIndices[tri * 3u], i1 = rtIndices[tri * 3u + 1u], i2 = rtIndices[tri * 3u + 2u];
		vec3 p0 = rtPosition(i0), p1 = rtPosition(i1), p2 = rtPosition(i2);
		vec3 w = vec3(1.0 - ls.uv.x - ls.uv.y, ls.uv.x, ls.uv.y);
		vec3 lightPos = w.x * p0 + w.y * p1 + w.z * p2;

		vec3 toLight = lightPos - s.pos;
		float distSq = max(dot(toLight, toLight), 1e-8);
		float dist = sqrt(distSq);
		vec3 L = toLight / dist;

		vec3 lightNormal = normalize(cross(p1 - p0, p2 - p0));
		// Closed emitters radiate outward only — their far half can never
		// see the receiver; open surfaces radiate from both sides.
		float sideSign = rtEmissive[index].sideSign;
		float cosLight = (sideSign == 0.0)
			? abs(dot(lightNormal, L))
			: max(dot(lightNormal * sideSign, -L), 0.0);
		if (cosLight <= 0.0)
			return vec3(0.0);

		int materialIndex = floatBitsToInt(rtAttribs[i0].normalMat.w);
		vec3 Le = rtMaterials[materialIndex].emissive.rgb;

		shadowDir = L;
		shadowDist = dist;
		// Area-measure contribution: brdf*cos * Le * cosLight / r^2 (the
		// source pdf is also in area measure: 1/(count*area)).
		return rtBrdfCos(s, V, L) * Le * (cosLight / distSq);
	}
	if (kind == RT_LIGHT_ENV) {
		vec3 L = rtOctDecode(ls.uv);
		shadowDir = L;
		shadowDist = 1e30;
		float NoL = max(dot(s.normal, L), 0.0);
		// Diffuse only: specular environment light always comes from the
		// prefiltered-map ambient term (cosine samples can't resolve sharp
		// lobes, and counting both would double the energy).
		vec3 diffuse = s.albedo * (1.0 - s.metallic) / RT_PI;
		return diffuse * rtEnvRadiance(L, 1) * NoL;
	}
	return vec3(0.0);
}

float rtTargetPdf(RTSurface s, vec3 V, RTLightSample ls)
{
	vec3 shadowDir;
	float shadowDist;
	return rtLuminance(rtLightContribution(s, V, ls, shadowDir, shadowDist));
}

// Streaming reservoir update; returns true when the candidate was selected.
bool rtUpdateReservoir(inout RTReservoir r, RTLightSample cand, float weight, inout uint rng, inout float wSum)
{
	wSum += weight;
	r.M += 1.0;
	if (weight > 0.0 && rtRand(rng) * wSum <= weight) {
		r.s = cand;
		return true;
	}
	return false;
}

// Whether environment light is sampled as direct light (only when the
// radiance cascades aren't running — they carry env light with visibility).
bool rtEnvAsDirectLight()
{
	return u.gi.z < 0.5 && lighting.params0.z > 0.0;
}

// Deterministic direct lighting from the (few) analytic directional lights,
// one shadow ray each — noise-free, unlike running them through ReSTIR.
vec3 rtAnalyticDirLights(RTSurface s, vec3 V)
{
	vec3 radiance = vec3(0.0);
	int dirCount = int(lighting.params0.x + 0.5);
	for (int i = 0; i < 4; i++) {
		if (i >= dirCount) break;
		vec3 L = -normalize(lighting.dirLights[i].direction.xyz);
		vec3 contribution = rtBrdfCos(s, V, L) * lighting.dirLights[i].color.rgb;
		if (dot(contribution, contribution) <= 0.0)
			continue;
		float eps = rtRayEpsilon(s.pos);
		if (rtOccluded(s.pos + s.geoNormal * eps, L, eps, 1e30))
			continue;
		radiance += contribution;
	}
	return radiance;
}

// Generate one light-sample candidate; returns its source pdf (mixed
// discrete/area/solid-angle measure, matching rtLightContribution).
// Directional lights are NOT in the pool: with only a handful of analytic
// key lights, shading them deterministically (rtAnalyticDirLights) is
// noise-free, and ReSTIR is reserved for the actual many-light problem
// (emissive triangles, points, environment).
RTLightSample rtGenerateCandidate(RTSurface s, inout uint rng, out float sourcePdf)
{
	int pointCount = int(lighting.params0.y + 0.5);
	int emissiveCount = int(u.misc.x + 0.5);
	bool envOn = rtEnvAsDirectLight();

	int classCount = pointCount + (emissiveCount > 0 ? 1 : 0) + (envOn ? 1 : 0);
	RTLightSample ls;
	ls.key = RT_LIGHT_INVALID;
	ls.uv = vec2(0.0);
	sourcePdf = 1.0;
	if (classCount == 0)
		return ls;

	int pick = min(int(rtRand(rng) * float(classCount)), classCount - 1);
	float classPdf = 1.0 / float(classCount);

	if (pick < pointCount) {
		ls.key = (RT_LIGHT_POINT << 28u) | uint(pick);
		sourcePdf = classPdf;
		return ls;
	}
	pick -= pointCount;
	if (emissiveCount > 0 && pick == 0) {
		int tri = min(int(rtRand(rng) * float(emissiveCount)), emissiveCount - 1);
		float u1 = rtRand(rng), u2 = rtRand(rng);
		float su = sqrt(u1);
		ls.key = (RT_LIGHT_EMISSIVE << 28u) | uint(tri);
		ls.uv = vec2(su * (1.0 - u2), su * u2); // barycentrics (b1, b2)
		sourcePdf = classPdf / (float(emissiveCount) * max(rtEmissive[tri].area, 1e-12));
		return ls;
	}
	// Environment: cosine-weighted around the shading normal.
	float u1 = rtRand(rng), u2 = rtRand(rng);
	float r = sqrt(u1);
	float phi = 2.0 * RT_PI * u2;
	vec3 t, b;
	rtBasis(s.normal, t, b);
	vec3 L = normalize(t * (r * cos(phi)) + b * (r * sin(phi)) + s.normal * sqrt(max(0.0, 1.0 - u1)));
	ls.key = (RT_LIGHT_ENV << 28u);
	ls.uv = rtOctEncode(L);
	sourcePdf = classPdf * max(dot(s.normal, L), 1e-4) / RT_PI;
	return ls;
}

// ---------------------------------------------------------------------------
// Depth helpers (shared with the raster passes' conventions)
// ---------------------------------------------------------------------------

// Metal depth is z_ndc in [0,1] directly; viewZ positive forward.
float rtLinearViewDepth(float depthValue)
{
	return u.proj.w / (depthValue + u.proj.z);
}
