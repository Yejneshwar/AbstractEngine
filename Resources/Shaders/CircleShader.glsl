#type vertex
#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aCirclePos;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec4 aColor;
layout(location = 4) in float aRadius;

#include <Resources/Shaders/GLBufferDeclarations.h>

layout(location = 0) out vec3 FragNormal;
layout(location = 1) out vec3 FragPosition;
layout(location = 2) out vec3 CirclePosition;
layout(location = 3) out vec4 Color;
layout(location = 4) out float Radius;


void main()
{
    FragNormal = mat3(transpose(inverse(ubo.viewMatrix))) * aNormal;
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

#include <Resources/Shaders/GLBufferDeclarations.h>
#include <Resources/Shaders/Text.h>

layout(location = 0) out vec4 FragColor;

void main()
{
    vec4 triangleColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);

    //maybe Perfect for back and front color
    //vec3 fragPos = vec3(ubo.viewMatrix * vec4(gl_FragCoord.xyz, 1.0));
	//vec2 uv = (( fragPos.xy -.5* ubo.aspectRatio)/881);


    //calculate distance from the origin point

    float d = length(FragPosition-CirclePosition);
    float col = smoothstep(Radius,Radius - 0.01 ,d) ;

    FragColor = vec4(Color.xyz*vec3(col), Color.a*col);

}