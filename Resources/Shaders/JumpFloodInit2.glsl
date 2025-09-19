#type vertex
#version 450 core

#include <Resource/Shaders/GridParameters.h>

layout(location = 0) out vec2 UV;

void main()
{
	int idx = indices[gl_VertexID];
	vec4 pos = vec4(pos2D[idx], 1.0);
	UV = tex[idx];

	gl_Position = pos;
}

#type fragment
#version 450 core

layout(location = 0) in vec2 UV;

layout(binding = 2) uniform sampler2D tex;

layout(location = 0) out vec4 FragColor;


void main()
{
	FragColor = texture(tex, UV);
}
