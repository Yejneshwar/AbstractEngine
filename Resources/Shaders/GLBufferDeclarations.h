layout(std140, binding = 0) uniform UBOCamera {
    mat4 view;
    mat4 projection;
	vec4 cameraPos;
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
