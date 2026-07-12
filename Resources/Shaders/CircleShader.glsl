#type vertex
#version 450 core
layout(location = 0) in int aID;
layout(location = 1) in vec3 aPos;
layout(location = 2) in vec3 aCirclePos;
layout(location = 3) in vec3 aNormal;
layout(location = 4) in vec4 aColor;
layout(location = 5) in float aRadius;

#include <Resource/Shaders/GLBufferDeclarations.h>

layout(location = 0) out vec3 FragNormal;
layout(location = 1) out vec3 FragPosition;
layout(location = 2) out vec3 CirclePosition;
layout(location = 3) out vec4 Color;
layout(location = 4) out float Radius;
layout(location = 5) out flat int  FragID;


void main()
{
    FragID = aID;
    // World-space normal (matches the PBR mesh Debug Normals view).
    FragNormal = aNormal;
    FragPosition = aPos;
    gl_Position = ubo.projViewMatrix * vec4(aPos, 1.0);
    CirclePosition = aCirclePos;
    Radius = aRadius;
    Color = aColor;
}

#type fragment
#version 450 core

layout(location = 0) in vec3 FragNormal;
layout(location = 1) in vec3 FragPosition;
layout(location = 2) in vec3 CirclePosition;
layout(location = 3) in vec4 Color;
layout(location = 4) in float Radius;
layout(location = 5) in flat int  FragID;

#include <Resource/Shaders/GLBufferDeclarations.h>
#include <Resource/Shaders/LightingDeclarations.h>

layout(location = 0) out vec4 FragColor;
layout(location = 1) out int FID;

void main()
{
    vec4 triangleColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);

    //maybe Perfect for back and front color
    //vec3 fragPos = vec3(ubo.viewMatrix * vec4(gl_FragCoord.xyz, 1.0));
    //vec2 uv = (( fragPos.xy -.5* ubo.aspectRatio)/881);


    //calculate distance from the origin point

    float d = length(FragPosition - CirclePosition);
    float col = smoothstep(Radius, Radius - 0.01, d);

    if (col == 0.0)
        discard;

    FID = FragID;

    // Debug Normals viewport mode: show the disc's normal as color
    // (zero-length normals from callers that don't set one render black).
    if (int(lighting.params1.x + 0.5) == SHADING_NORMALS) {
        vec3 n = dot(FragNormal, FragNormal) > 1e-8 ? normalize(FragNormal) : vec3(-1.0);
        FragColor = vec4(srgbToLinear(n * 0.5 + 0.5), Color.a * col);
        return;
    }

    FragColor = vec4(Color.xyz * vec3(col), Color.a * col);
    // Linear-light viewports (HDR + tonemap): linearize sRGB-authored colors.
    if (ubo.outputLinear != 0)
        FragColor.rgb = pow(max(FragColor.rgb, vec3(0.0)), vec3(2.2));
}
