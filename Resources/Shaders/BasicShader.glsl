#type vertex
#version 450 core
layout(location = 0) in int aID;
layout(location = 1) in vec3 aPos;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec3 aColor;

#include <Resource/Shaders/GLBufferDeclarations.h>

// OUTPUTS to Fragment Shader
layout(location = 0) out vec3 vNormal;
layout(location = 1) out vec3 vFragPos;
layout(location = 2) out vec3 vBarycentric;
layout(location = 3) out flat int vID;

void main()
{
    // Pass-through standard vertex data
    vNormal = mat3(transpose(inverse(ubo.viewMatrix))) * aNormal;
    vFragPos = vec3(ubo.viewMatrix * vec4(aPos, 1.0));
    vID = aID;
    
    // Generate barycentric coordinates on-the-fly using the vertex index
    int prim_index = gl_VertexID % 3;
    if (prim_index == 0) {
        vBarycentric = vec3(1.0, 0.0, 0.0);
    } else if (prim_index == 1) {
        vBarycentric = vec3(0.0, 1.0, 0.0);
    } else { // prim_index == 2
        vBarycentric = vec3(0.0, 0.0, 1.0);
    }
    
    // Standard position calculation
    gl_Position = ubo.projViewMatrix * vec4(aPos, 1.0);
}

#type fragment
#version 450 core
#extension GL_ARB_shader_stencil_export : enable

// INPUTS from Vertex Shader
layout(location = 0) in vec3 vNormal;
layout(location = 1) in vec3 vFragPos;
layout(location = 2) in vec3 vBarycentric;
layout(location = 3) in flat int vID;

// OUTPUTS
layout(location = 0) out vec4 FragColor;
layout(location = 1) out int FragID_out;
layout(location = 2) out vec4 FragColor2;

#include <Resource/Shaders/GLBufferDeclarations.h>

layout(std140, binding = 1) uniform UBOFragmentAttached{
    vec4 triangleColor;
    int selectedObject;
} uboa;

float amplify(float d, float scale, float offset)
{
    d = scale * d + offset;
    d = clamp(d, 0, 1);
    d = 1 - exp2(-2 * d * d);
    return d;
}

void main()
{
    vec4 objectColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
    float dotProductFrag = dot(normalize(vNormal), normalize(vFragPos.xyz - vec3(0.0, 0.0, 1.0)));

    // Find the smallest barycentric coordinate to determine distance to the edge
    float d1 = min(min(vBarycentric.x, vBarycentric.y), vBarycentric.z);
    
    // Use the distance to create the wireframe effect
    vec4 wireframeColor = amplify(d1, 40, -0.5) * objectColor;

    vec3 finalColor = dotProductFrag > 0.0 ? wireframeColor.xyz : vec3(0.4, 0.1, 0.8);

    FragColor = vec4(finalColor, objectColor.a);
    FragID_out = vID;
}
