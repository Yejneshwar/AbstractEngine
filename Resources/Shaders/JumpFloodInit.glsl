#type vertex
#version 450 core

#include <Resource/Shaders/GridParameters.h>


void main()
{
	int idx = indices[gl_VertexID];

	gl_Position = vec4(pos2D[idx], 1.0);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 FragColor;

void main()
{

	FragColor = vec4(1);
}
