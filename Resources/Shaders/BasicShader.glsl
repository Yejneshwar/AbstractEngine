#type vertex
#version 450 core
layout(location = 0) in int aID;
layout(location = 1) in vec3 aPos;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec3 aColor;

#include <Resources/Shaders/GLBufferDeclarations.h>

layout(location = 0) out vec3 FragNormal;
layout(location = 1) out vec3 FragPosition;
layout(location = 2) out flat int  FragID;

void main()
{
    FragNormal = mat3(transpose(inverse(ubo.viewMatrix))) * aNormal;
    FragPosition = vec3(ubo.viewMatrix * vec4(aPos, 1.0));
    gl_Position = ubo.projViewMatrix * vec4(aPos, 1.0);
    FragID = aID;
}

#type geometry
#version 450 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) in vec3 FragNormal[3]; // Input normal from vertex shader
layout(location = 1) in vec3 FragPosition[3]; // Input position from vertex shader
layout(location = 2) in flat int FragID[3]; // Input ID from vertex shader

layout(location = 0) out vec3 GeomFragNormal[3]; // Output normal for passing to fragment shader
layout(location = 4) out vec3 GeomFragPosition[3]; // Output position for passing to fragment shader
layout(location = 7) out vec3 gTriDistance;
layout(location = 8) out flat int GeomFragID;

void main()
{
    GeomFragID = FragID[0];
    GeomFragNormal[0] = FragNormal[0];
    GeomFragNormal[1] = FragNormal[1];
    GeomFragNormal[2] = FragNormal[2];

    GeomFragPosition[0] = FragPosition[0];
    GeomFragPosition[1] = FragPosition[1];
    GeomFragPosition[2] = FragPosition[2];

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
#extension GL_ARB_shader_stencil_export : enable

layout(location = 0) in vec3 GeomFragNormal;
layout(location = 4) in vec3 GeomFragPosition;
layout(location = 7) in vec3 gTriDistance;
layout(location = 8) in flat int GeomFragID;

layout(location = 0) out vec4 FragColor;
layout(location = 1) out int FragID;
layout(location = 2) out vec4 FragColor2;

#include <Resources/Shaders/GLBufferDeclarations.h>

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
    vec4 triangleColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
    float dotProductFrag = dot(normalize(GeomFragNormal), normalize(GeomFragPosition.xyz - vec3(0.0, 0.0, 1.0)));

    float d1 = min(min(gTriDistance.x, gTriDistance.y), gTriDistance.z);
    vec4 color = amplify(d1, 40, -0.5) * triangleColor;

    vec3 finalColor = dotProductFrag > 0.0 ? color.xyz : vec3(0.4, 0.1, 0.8);

    FragColor = vec4(finalColor, triangleColor.a);
    FragID = GeomFragID;
}