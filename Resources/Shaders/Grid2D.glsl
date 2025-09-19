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
	vec3 position = pos2D[idx];
	
	out_camPos = ubo.cameraPos.xy;

	gl_Position = vec4(pos2D[idx], 1.0);
	uv = (inverse(MVP) * vec4(position, 1.0)).xy;
}

#type fragment
#version 460 core

#include <Resource/Shaders/GLBufferDeclarations.h>
#include <Resource/Shaders/GridParameters.h>
#include <Resource/Shaders/GridCalculation.h>
#include <Resource/Shaders/Text.h>

layout (location=0) in vec2 uv;
layout (location=1) in vec2 camPos;
layout (location=0) out vec4 out_FragColor;

void main()
{
	mat4 transform = inverse(ubo.viewMatrix);
	float FontSize = ubo.gridMajor * 2;
	vec4 textColor = vec4(0.0,0.0,0.0,0.0);
	float zoom = ubo.gridZoom;
	vec4 GMin = vec4(ubo.gridMinMax.x,ubo.gridMinMax.z * ubo.aspectRatio,0.0,1.0);
	vec4 GMax = vec4(ubo.gridMinMax.y,ubo.gridMinMax.w * ubo.aspectRatio,0.0,1.0);
	float panXint = 0.0;
	float panXFrac = modf(camPos.x, panXint);
	float panYint = 0.0;
	float panYFrac = modf(camPos.y, panYint);
	vec2 gMin = (transform * GMin).xy;
	vec2 gMax = (transform * GMax).xy;
	float Xoffset = mod(zoom-panXFrac,ubo.gridMajor);
	float Yoffset = mod(zoom-panYFrac,ubo.gridMajor);

	float stepSize = ubo.gridMajor;
    float Xstart = gMin.x + Xoffset - (mod(panXint,ubo.gridMajor));
    float Xend = gMax.x;
	float Ystart = gMin.y + Yoffset - (mod(panYint,ubo.gridMajor));
	float Yend = gMax.y;

	for (float i = Ystart; i < Yend; i += stepSize){
		vec2 position = vec2(0,i);
		vec2 U = ( uv - position )*(32)/FontSize;
		textColor += pFloat(U, position.y);
	}

	for (float i = Xstart; i < Xend; i += stepSize){
		vec2 position = vec2(i,0);
		vec2 U = ( uv - position )*(32)/FontSize;
		textColor += pFloat(U, position.x);
	}
	out_FragColor = gridColor(uv, camPos) + textColor.xxxx;
};
