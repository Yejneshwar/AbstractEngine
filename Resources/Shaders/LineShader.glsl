#type vertex
#version 450 core
layout(location = 0) in int aID;
layout(location = 1) in vec3 aPos;
layout(location = 2) in vec4 aColor;

layout(location = 0) out vec4 vColor;
layout(location = 1) out flat int  FragID;

#include <Resource/Shaders/GLBufferDeclarations.h>

void main()
{
    FragID = aID;
    gl_Position = ubo.projViewMatrix * vec4(aPos, 1.0);
    vColor = aColor;
}

#type fragment
#version 450 core

layout(location = 0) in vec4 vColor;
layout(location = 1) in flat int  FragID;

#include <Resource/Shaders/GLBufferDeclarations.h>

layout(location = 0) out vec4 FragColor;
layout(location = 1) out int  FID;


void main()
{
    FID = FragID;
    FragColor = vColor;
    // Linear-light viewports (HDR + tonemap): linearize sRGB-authored colors.
    if (ubo.outputLinear != 0)
        FragColor.rgb = pow(max(FragColor.rgb, vec3(0.0)), vec3(2.2));
}
