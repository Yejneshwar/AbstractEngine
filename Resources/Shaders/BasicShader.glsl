#type vertex
#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

#include <Resources/Shaders/GLBufferDeclarations.h>

void main()
{
    gl_Position = ubo.projViewMatrix * vec4(aPos, 1.0);
}

#type geometry
#version 450 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 1) out vec3 gTriDistance;

void main()
{

    gTriDistance = vec3(1, 0, 0);
    gl_Position = gl_in[0].gl_Position; EmitVertex();

    gTriDistance = vec3(0, 1, 0);
    gl_Position = gl_in[1].gl_Position; EmitVertex();

    gTriDistance = vec3(0, 0, 1);
    gl_Position = gl_in[2].gl_Position; EmitVertex();

    EndPrimitive();
}

#type fragment
#version 450 core

layout(location = 1) in vec3 gTriDistance;

layout(location = 0) out vec4 FragColor;

layout(std140, binding = 1) uniform UBOFragment {
    vec4 triangleColor;
} ubo;

float amplify(float d, float scale, float offset)
{
    d = scale * d + offset;
    d = clamp(d, 0, 1);
    d = 1 - exp2(-2 * d * d);
    return d;
}

void main()
{
    float d1 = min(min(gTriDistance.x, gTriDistance.y), gTriDistance.z);
    vec4 color = amplify(d1, 40, -0.5) * ubo.triangleColor;

    FragColor = vec4(color.xyz, ubo.triangleColor.a);
}