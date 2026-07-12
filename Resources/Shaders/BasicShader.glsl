#type vertex
#version 450 core
layout(location = 0) in int aID;
layout(location = 1) in vec3 aPos;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec3 aColor;

#include <Resource/Shaders/GLBufferDeclarations.h>

// OUTPUTS to Fragment Shader
layout(location = 0) out vec3 vNormal;      // view space
layout(location = 1) out vec3 vFragPos;     // view space
layout(location = 2) out vec3 vBarycentric;
layout(location = 3) out vec3 vColor;
layout(location = 4) out flat int vID;

void main()
{
    vNormal = mat3(transpose(inverse(ubo.viewMatrix))) * aNormal;
    vFragPos = vec3(ubo.viewMatrix * vec4(aPos, 1.0));
    vColor = aColor;
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

    gl_Position = ubo.projViewMatrix * vec4(aPos, 1.0);
}

#type fragment
#version 450 core

// INPUTS from Vertex Shader
layout(location = 0) in vec3 vNormal;
layout(location = 1) in vec3 vFragPos;
layout(location = 2) in vec3 vBarycentric;
layout(location = 3) in vec3 vColor;
layout(location = 4) in flat int vID;

// OUTPUTS (no FragColor2: this shader must not touch the selection-mask
// attachment — the dedicated SelectedObject pass owns it)
layout(location = 0) out vec4 FragColor;
layout(location = 1) out int FragID_out;

#include <Resource/Shaders/GLBufferDeclarations.h>
#include <Resource/Shaders/LightingDeclarations.h>

// Selection is handled by the dedicated SelectedObject pass, so this block
// only carries the edge color (must stay layout-compatible with the
// 16-byte UBODataFragment the BatchRenderer binds at binding 1).
layout(std140, binding = 1) uniform UBOFragmentAttached{
    vec4 triangleColor; // wireframe edge color
} uboa;

void main()
{
    FragID_out = vID;

    // Proper facing test: view vector is from the fragment to the camera
    // (origin in view space). The old code dotted the normal against the
    // direction from a fixed point, which flipped arbitrarily with the view.
    vec3 N = normalize(vNormal);
    vec3 V = normalize(-vFragPos);
    float facing = dot(N, V);
    bool frontFacing = facing >= 0.0;

    // Debug Normals viewport mode (view-space normals for this shader).
    if (int(lighting.params1.x + 0.5) == SHADING_NORMALS) {
        vec3 shown = N * 0.5 + 0.5;
        FragColor = vec4(ubo.outputLinear != 0 ? srgbToLinear(shown) : shown, 1.0);
        return;
    }

    // Simple headlight shade on the vertex color; back faces get the
    // classic diagnostic purple, dimmed by the same shading term.
    float shade = 0.35 + 0.65 * abs(facing);
    vec3 fill = frontFacing ? vColor * shade : vec3(0.4, 0.1, 0.8) * shade;

    // Screen-space-derivative wireframe: crisp ~1px edges at any distance
    // (the old exp2 falloff got thick up close and vanished far away).
    float edgeDistance = min(min(vBarycentric.x, vBarycentric.y), vBarycentric.z);
    float line = 1.0 - smoothstep(0.0, fwidth(edgeDistance) * 1.5, edgeDistance);

    vec3 color = mix(fill, uboa.triangleColor.rgb, line);
    if (ubo.outputLinear != 0)
        color = srgbToLinear(color);

    FragColor = vec4(color, 1.0);
}
