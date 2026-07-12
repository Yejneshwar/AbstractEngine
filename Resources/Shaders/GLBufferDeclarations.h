#define UBO_SCENE 0

layout(std140, binding = UBO_SCENE) uniform SceneDataUBO {
	mat4 projViewMatrix;
	mat4 viewMatrix;
	mat4 viewMatrixInverse;
	mat4 viewMatrixInverseTranspose;
	mat4 projectionMatrix;
	mat4 projectionMatrixInverse;
	vec4 cameraPos;
	vec4 viewDirection;
	vec4 gridMinMax;
	vec4 viewport; // (width, height, width*height, 0)
	float aspectRatio;
	float gridMajor;
	float gridMinor;
	float gridZoom;
	int selectedObject;
	// 1 when the viewport renders linear HDR and the tonemap post chain
	// runs (3D viewports): shaders must linearize their sRGB-authored colors.
	int outputLinear;


	//ivec3 viewport;  // (width, height, width*height)
	// For SIMPLE, INTERLOCK, SPINLOCK, LOOP, and LOOP64, the number of OIT layers;
	// for LINKEDLIST, the total number of elements in the A-buffer.
	// uint linkedListAllocatedPerElement;

	// float alphaMin;
	// float alphaWidth;
	//vec2  _pad1;
} ubo;

// NOTE: the old unused Vertices/Matrices SSBO declarations (std430 bindings
// 1/2) were removed — on Metal, SSBOs and UBOs share the [[buffer(n)]] index
// space and they aliased the fragment UBO (1) and lighting UBO (2).
