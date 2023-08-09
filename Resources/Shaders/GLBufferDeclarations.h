#define UBO_SCENE 0

layout(std140, binding = UBO_SCENE) uniform SceneDataUBO {
	mat4 projViewMatrix;
	mat4 viewMatrix;
	mat4 projectionMatrix;
	mat4 viewMatrixInverseTranspose;
	vec4 cameraPos;

	ivec3 viewport;  // (width, height, width*height)
	// For SIMPLE, INTERLOCK, SPINLOCK, LOOP, and LOOP64, the number of OIT layers;
	// for LINKEDLIST, the total number of elements in the A-buffer.
	uint linkedListAllocatedPerElement;

	float alphaMin;
	float alphaWidth;
	vec2  _pad1;
} ubo;

struct Vertex
{
	float p[3];
	float n[3];
	float tc[2];
};

layout(std430, binding = 1) restrict readonly buffer Vertices
{
	Vertex in_Vertices[];
};

layout(std430, binding = 2) restrict readonly buffer Matrices
{
	mat4 in_ModelMatrices[];
};
