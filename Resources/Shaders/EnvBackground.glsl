#type vertex
#version 450 core

// Fullscreen pass drawn via Renderer::DrawGridTriangles (6 vertices, no
// vertex buffer). Renders the environment as the 3D viewport background and
// explicitly resets the picking ID (-1) and selection mask under it.

#include <Resource/Shaders/GLBufferDeclarations.h>

layout(location = 0) out vec2 vNdc;

const vec2 kFullscreen[6] = vec2[](
    vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0),
    vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, 1.0)
);

void main()
{
    vNdc = kFullscreen[gl_VertexID];
    gl_Position = vec4(vNdc, 1.0, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) in vec2 vNdc;

#include <Resource/Shaders/GLBufferDeclarations.h>
#include <Resource/Shaders/LightingDeclarations.h>

layout(binding = 4) uniform sampler2D envSpecular;

layout(location = 0) out vec4 FragColor;
layout(location = 1) out int FID;
layout(location = 2) out vec4 FragColor2;

void main()
{
    // Unproject the pixel to a world-space view ray. Using the projection
    // inverse keeps this correct on Metal, where the projection carries the
    // Y flip.
    vec4 viewTarget = ubo.projectionMatrixInverse * vec4(vNdc, 1.0, 1.0);
    vec3 worldDir = normalize(mat3(ubo.viewMatrixInverse) * (viewTarget.xyz / viewTarget.w));

    float blurLod = lighting.params1.z;
    vec3 env = textureLod(envSpecular, dirToEquirect(worldDir), blurLod).rgb;

    FragColor = vec4(env * lighting.params0.z, 1.0);
    FID = -1;
    FragColor2 = vec4(0.0);
}
