#type vertex
#version 450 core

// Wireframe pass for retained meshes: same vertex layout as PBRMesh, drawn
// with line polygon/triangle-fill mode by the BatchRenderer (wire-only
// pipeline or the Solid-mode overlay).

layout(location = 0) in int aID;
layout(location = 1) in vec3 aPos;

#include <Resource/Shaders/GLBufferDeclarations.h>

layout(location = 0) out flat int FragID;

void main()
{
    FragID = aID;
    gl_Position = ubo.projViewMatrix * vec4(aPos, 1.0);
    // Small NDC depth bias toward the camera so overlay edges win the depth
    // test against the fill they sit on.
    gl_Position.z -= 1e-4 * gl_Position.w;
}

#type fragment
#version 450 core

layout(location = 0) in flat int FragID;

#include <Resource/Shaders/GLBufferDeclarations.h>
#include <Resource/Shaders/LightingDeclarations.h>

layout(location = 0) out vec4 FragColor;
layout(location = 1) out int FID;

void main()
{
    FID = FragID;
    vec3 color = lighting.params2.rgb;
    if (ubo.outputLinear != 0)
        color = srgbToLinear(color);
    FragColor = vec4(color, 1.0);
}
