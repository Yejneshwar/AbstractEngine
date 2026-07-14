#type vertex
#version 450 core
layout(location = 0) in int aID;
layout(location = 1) in vec3 aPos;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec4 aColor;
layout(location = 4) in int aMaterial;

#include <Resource/Shaders/GLBufferDeclarations.h>

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec4 vColor;
layout(location = 3) out flat int FragID;
layout(location = 4) out flat int vMaterial;

void main()
{
    FragID = aID;
    vWorldPos = aPos; // mesh data is authored in world space (no model matrix)
    vNormal = aNormal;
    vColor = aColor;
    vMaterial = aMaterial;
    gl_Position = ubo.projViewMatrix * vec4(aPos, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec4 vColor;
layout(location = 3) in flat int FragID;
layout(location = 4) in flat int vMaterial;

#include <Resource/Shaders/GLBufferDeclarations.h>
#include <Resource/Shaders/LightingDeclarations.h>

// GGX-prefiltered equirect environment (mip = roughness) and the matcap
// used by the low-power viewport shading mode.
layout(binding = 4) uniform sampler2D envSpecular;
layout(binding = 5) uniform sampler2D matcapTex;

layout(location = 0) out vec4 FragColor;
layout(location = 1) out int FID;

// ---------------------------------------------------------------------------
// BRDF (glTF metallic-roughness: Lambert diffuse + GGX specular,
// height-correlated Smith visibility, Schlick Fresnel)
// ---------------------------------------------------------------------------

float D_GGX(float NoH, float a)
{
    float a2 = a * a;
    float d = (NoH * a2 - NoH) * NoH + 1.0;
    return a2 / (PI * d * d);
}

float V_SmithGGXCorrelated(float NoV, float NoL, float a)
{
    float a2 = a * a;
    float gv = NoL * sqrt(NoV * NoV * (1.0 - a2) + a2);
    float gl = NoV * sqrt(NoL * NoL * (1.0 - a2) + a2);
    return 0.5 / max(gv + gl, 1e-5);
}

vec3 F_Schlick(vec3 f0, float VoH)
{
    float f = pow(1.0 - VoH, 5.0);
    return f0 + (vec3(1.0) - f0) * f;
}

// Lazarov's analytic environment BRDF approximation (split-sum scale/bias
// without a LUT).
vec3 EnvBRDFApprox(vec3 f0, float roughness, float NoV)
{
    const vec4 c0 = vec4(-1.0, -0.0275, -0.572, 0.022);
    const vec4 c1 = vec4(1.0, 0.0425, 1.04, -0.04);
    vec4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * NoV)) * r.x + r.y;
    vec2 ab = vec2(-1.04, 1.04) * a004 + r.zw;
    return f0 * ab.x + ab.y;
}

vec3 shadeLight(vec3 albedo, float metallic, vec3 f0, float a,
                vec3 N, vec3 V, float NoV, vec3 L, vec3 radiance)
{
    float NoL = dot(N, L);
    if (NoL <= 0.0)
        return vec3(0.0);

    vec3 H = normalize(V + L);
    float NoH = clamp(dot(N, H), 0.0, 1.0);
    float VoH = clamp(dot(V, H), 0.0, 1.0);

    vec3 F = F_Schlick(f0, VoH);
    vec3 specular = D_GGX(NoH, a) * V_SmithGGXCorrelated(NoV, NoL, a) * F;
    vec3 diffuse = albedo * (1.0 - metallic) * (vec3(1.0) - F) / PI;

    return (diffuse + specular) * radiance * NoL;
}

void main()
{
    FID = FragID;

    GpuMaterial material = materialTable.materials[vMaterial];
    int flags = int(material.mrfx.z + 0.5);
    int shadingMode = int(lighting.params1.x + 0.5);

    vec4 base = material.baseColor;
    if ((flags & MATERIAL_FLAG_VERTEX_TINT) != 0)
        base *= vColor;

    if (shadingMode == SHADING_UNLIT) {
        // Legacy behavior for unlit scenes; when the HDR post chain runs,
        // work in linear so tonemap+encode round-trips the color.
        if (lighting.params1.y > 0.5)
            base.rgb = srgbToLinear(base.rgb);
        FragColor = base;
        return;
    }

    vec3 V = normalize(ubo.cameraPos.xyz - vWorldPos);

    vec3 N;
    if ((flags & MATERIAL_FLAG_FLAT_NORMALS) != 0 || dot(vNormal, vNormal) < 0.25)
        N = normalize(cross(dFdx(vWorldPos), dFdy(vWorldPos)));
    else
        N = normalize(vNormal);
    // CAD meshes are double-sided: always shade the side facing the viewer.
    if (dot(N, V) < 0.0)
        N = -N;

    if (shadingMode == SHADING_NORMALS) {
        // Debug view: world normals mapped to [0,1]. When the HDR post chain
        // runs it re-encodes to sRGB, so pre-linearize; without it (2D
        // viewports) output display-referred directly.
        vec3 shown = N * 0.5 + 0.5;
        FragColor = vec4(lighting.params1.y > 0.5 ? srgbToLinear(shown) : shown, base.a);
        return;
    }

    vec3 albedo = srgbToLinear(base.rgb);

    if (shadingMode == SHADING_MATCAP) {
        vec3 nView = normalize(mat3(ubo.viewMatrix) * N);
        vec3 matcap = srgbToLinear(texture(matcapTex, nView.xy * 0.5 + 0.5).rgb);
        vec3 solid = matcap * albedo * 2.0;
        // Without the tonemap pass (2D viewports), gamma-encode here.
        if (lighting.params1.y <= 0.5)
            solid = pow(clamp(solid, 0.0, 1.0), vec3(1.0 / 2.2));
        FragColor = vec4(solid, base.a);
        return;
    }

    float metallic = clamp(material.mrfx.x, 0.0, 1.0);
    float roughness = clamp(material.mrfx.y, 0.045, 1.0);
    float a = roughness * roughness;
    vec3 f0 = mix(vec3(0.04), albedo, metallic);
    float NoV = clamp(dot(N, V), 1e-4, 1.0);

    vec3 color = vec3(0.0);

    int dirCount = int(lighting.params0.x + 0.5);
    for (int i = 0; i < MAX_DIR_LIGHTS; ++i) {
        if (i >= dirCount) break;
        vec3 L = -normalize(lighting.dirLights[i].direction.xyz);
        color += shadeLight(albedo, metallic, f0, a, N, V, NoV, L, lighting.dirLights[i].color.rgb);
    }

    int pointCount = int(lighting.params0.y + 0.5);
    for (int i = 0; i < MAX_POINT_LIGHTS; ++i) {
        if (i >= pointCount) break;
        vec3 toLight = lighting.pointLights[i].position.xyz - vWorldPos;
        float distSq = max(dot(toLight, toLight), 1e-6);
        vec3 L = toLight * inversesqrt(distSq);
        float attenuation = 1.0 / distSq;
        float radius = lighting.pointLights[i].position.w;
        if (radius > 0.0) {
            // Smooth window so the light's influence actually reaches zero.
            float s = clamp(1.0 - pow(distSq / (radius * radius), 2.0), 0.0, 1.0);
            attenuation *= s * s;
        }
        color += shadeLight(albedo, metallic, f0, a, N, V, NoV, L,
                            lighting.pointLights[i].color.rgb * attenuation);
    }

    // Image-based lighting: SH irradiance for diffuse, prefiltered equirect
    // + analytic env BRDF for specular.
    float envIntensity = lighting.params0.z;
    if (envIntensity > 0.0) {
        vec3 irradiance = evalIrradianceSH(N);
        vec3 iblDiffuse = albedo * (1.0 - metallic) * irradiance;

        vec3 R = reflect(-V, N);
        float mipCount = lighting.params0.w;
        float mip = roughness * max(mipCount - 1.0, 0.0);
        vec3 prefiltered = textureLod(envSpecular, dirToEquirect(R), mip).rgb;
        vec3 iblSpecular = prefiltered * EnvBRDFApprox(f0, roughness, NoV);

        color += (iblDiffuse + iblSpecular) * envIntensity;
    }

    color += material.emissive.rgb;

    FragColor = vec4(color, base.a);
}
