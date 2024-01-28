#type vertex
#version 450 core

#include <Resources/Shaders/GridParameters.h>

layout(location = 0) out vec2 UV;

void main()
{
    int idx = indices[gl_VertexID];
    vec4 pos = vec4(pos2D[idx], 1.0);
    UV = tex[idx];

    gl_Position = pos;
}

#type fragment
#version 450 core

layout(location = 0) out vec4 FragColor;
layout(location = 1) out int FragID;


layout(location = 0) in vec2 UV;

layout(binding = 1) uniform sampler2D u_Texture;

void main()
{
    vec4 pixel = texture(u_Texture, UV);

    if (pixel.xyz == vec3(1.0f)) { discard; }
    if (pixel.a == 0.0f) { discard; }
    FragColor = pixel;
}