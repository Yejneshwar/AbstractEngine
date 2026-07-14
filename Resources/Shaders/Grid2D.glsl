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
	// ubo.gridMinMax now holds the TRUE world-space bounds of the view
	// (pan-inclusive, from the 2D camera) — labels iterate the visible
	// gridline values directly; no pan/zoom re-phasing needed.
	float xMin = ubo.gridMinMax.x;
	float xMax = ubo.gridMinMax.y;
	float yMin = ubo.gridMinMax.z;
	float yMax = ubo.gridMinMax.w;

	float stepSize = ubo.gridMajor;
	vec4 textColor = vec4(0.0);

	// Ruler behavior: numbers sit exactly on their gridline ALONG the ruler
	// axis; the transverse position rides the world axes when visible and
	// otherwise clamps smoothly to the viewport edge (no lattice snapping —
	// that popped a whole cell at a time while panning).
	// (stepSize guard: spacing is 0 until the camera computes it.)
	if (stepSize > 1e-30) {
		// Screen-constant glyph size (~12 pt) so labels stay readable at any
		// zoom (tying them to gridMajor made them breathe 10x per LOD decade).
		float worldPerPixel = (xMax - xMin) / max(ubo.viewport.x, 1.0);
		float glyphSize = 12.0 * worldPerPixel;
		// pFloat's U space is 32 units per glyph cell.
		float FontSize = 32.0 * glyphSize;

		float rowLo = yMin + 0.5 * glyphSize;
		float rowHi = max(rowLo, yMax - 2.0 * glyphSize);
		float labelRowY = clamp(0.0, rowLo, rowHi);

		float colLo = xMin + 0.5 * glyphSize;
		float colHi = max(colLo, xMax - 6.0 * glyphSize);
		float labelColX = clamp(0.0, colLo, colHi);

		for (float i = ceil(yMin / stepSize) * stepSize; i <= yMax; i += stepSize){
			vec2 U = ( uv - vec2(labelColX, i) )*(32)/FontSize;
			textColor += pFloat(U, i);
		}

		for (float i = ceil(xMin / stepSize) * stepSize; i <= xMax; i += stepSize){
			vec2 U = ( uv - vec2(i, labelRowY) )*(32)/FontSize;
			textColor += pFloat(U, i);
		}
	}

	// Fade radius scales with the view so the canvas grid never runs out.
	out_FragColor = gridColor(uv, camPos, 4.0 * (xMax - xMin)) + textColor.xxxx;
};
