// Shared lighting + material declarations for the lit (PBR) pipeline.
// Mirrored on the C++ side in Graphics/Renderer/Lighting.h — keep in sync.

#define UBO_LIGHTING 2
#define UBO_MATERIALS 3

#define MAX_DIR_LIGHTS 4
#define MAX_POINT_LIGHTS 8
#define MAX_MATERIALS 256

// lighting.params1.x
#define SHADING_UNLIT 0
#define SHADING_LIT 1
#define SHADING_MATCAP 2
#define SHADING_NORMALS 3 // debug view: world-space normals as color

// Material flag bits (materials[].mrfx.z)
#define MATERIAL_FLAG_FLAT_NORMALS 1
#define MATERIAL_FLAG_VERTEX_TINT 2

struct DirLight {
	vec4 direction; // xyz = direction light travels (normalized), w unused
	vec4 color;     // rgb premultiplied by intensity (linear)
};

struct PointLight {
	vec4 position;  // xyz, w = influence radius (<= 0: unbounded)
	vec4 color;     // rgb premultiplied by intensity (linear)
};

layout(std140, binding = UBO_LIGHTING) uniform LightingUBO {
	DirLight dirLights[MAX_DIR_LIGHTS];
	PointLight pointLights[MAX_POINT_LIGHTS];
	// Cosine-convolved irradiance SH of the environment, pre-packed so that
	// evalSH(n) directly yields E(n)/pi (the diffuse ambient term).
	vec4 shIrradiance[9];
	vec4 params0; // x = dirLightCount, y = pointLightCount, z = envIntensity, w = envSpecularMipCount
	vec4 params1; // x = shadingMode, y = outputLinear (1 = HDR->tonemap post runs), z = background blur lod, w unused
} lighting;

struct GpuMaterial {
	vec4 baseColor; // rgba (sRGB-authored; linearized in-shader for lit paths)
	vec4 mrfx;      // x = metallic, y = roughness, z = flags (int), w unused
	vec4 emissive;  // rgb premultiplied by intensity (linear)
};

layout(std140, binding = UBO_MATERIALS) uniform MaterialUBO {
	GpuMaterial materials[MAX_MATERIALS];
} materialTable;

// ---------------------------------------------------------------------------
// Shared helpers
// ---------------------------------------------------------------------------

const float PI = 3.14159265359;

vec3 srgbToLinear(vec3 c)
{
	return pow(max(c, vec3(0.0)), vec3(2.2));
}

// Equirectangular lookup. Row v=0 is the zenith (+Y); the CPU generator
// uploads data in the matching order per backend.
vec2 dirToEquirect(vec3 d)
{
	float u = atan(d.z, d.x) * 0.15915494309 + 0.5; // 1/(2*pi)
	float v = acos(clamp(d.y, -1.0, 1.0)) * 0.31830988618; // 1/pi
	return vec2(u, v);
}

// Diffuse ambient from the packed irradiance SH (already includes the cosine
// lobe and 1/pi — multiply by albedo directly).
vec3 evalIrradianceSH(vec3 n)
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
