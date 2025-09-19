#type vertex
#version 460 core

#include <Resource/Shaders/GLBufferDeclarations.h>
#include <Resource/Shaders/GridParameters.h>

layout (location=0) out vec2 uv;
layout (location=1) out vec2 out_camPos;

void main()
{
	mat4 MVP = ubo.projViewMatrix;

	int idx = indices[gl_VertexID];
	vec3 position = pos[idx] * 1000;


	out_camPos = ubo.cameraPos.xz;

	gl_Position = MVP * (vec4(position, 1.0));
	uv = position.xz;
}

#type fragment
#version 460 core

#include <Resource/Shaders/GridParameters.h>
#include <Resource/Shaders/GridCalculation.h>

layout (location=0) in vec2 uv;
layout (location=1) in vec2 camPos;
layout (location=0) out vec4 out_FragColor;

void main()
{
	out_FragColor = gridColor(uv, camPos);
};
