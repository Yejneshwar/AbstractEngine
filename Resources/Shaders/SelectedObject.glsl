#type vertex
#version 450 core
layout(location = 0) in int aID;
layout(location = 1) in vec3 aPos;

layout(location = 0) out flat int  FragID;

#include <Resources/Shaders/GLBufferDeclarations.h>

void main()
{
    FragID = aID;
    gl_Position = ubo.projViewMatrix * vec4(aPos, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) in flat int  FragID;

#include <Resources/Shaders/GLBufferDeclarations.h>

//layout(location = 1) out int FID;
layout(location = 2) out vec4 FragColor2;

void main()
{


    if((FragID == ubo.selectedObject) && (ubo.selectedObject != -1)){
        FragColor2 = vec4(1.0,1.0,1.0,1.0);
    }
    else {
        discard;
    }
}