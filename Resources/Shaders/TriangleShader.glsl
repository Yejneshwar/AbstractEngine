#type vertex
#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;

layout(location = 0) out vec4 vColor;

#include <Resources/Shaders/GLBufferDeclarations.h>

void main()
{
    gl_Position = ubo.projViewMatrix * vec4(aPos, 1.0);
    vColor = aColor;
}

#type fragment
#version 450 core

layout(location = 0) in vec4 vColor;

layout(location = 0) out vec4 FragColor;

void main()
{
    FragColor = vColor;
}