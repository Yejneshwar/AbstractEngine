#type vertex
#version 450 core
layout(location = 0) in vec3 aPos;
//layout(location = 1) in vec3 aColor;


#include <Resources/Shaders/GLBufferDeclarations.h>

void main()
{
    gl_Position = ubo.projViewMatrix * vec4(aPos, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0);
}