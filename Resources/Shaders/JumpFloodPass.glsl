#type vertex
#version 450 core

#include <Resources/Shaders/GridParameters.h>

layout(location = 2) uniform vec2 a_texelSize;
layout(location = 3) uniform float a_invTexelRatio;
layout(location = 4) uniform int a_step;


struct VertexOutput
{
    vec2 TexCoords;
    vec2 TexelSize;
    vec2 UV[9];
    float invTexelRatio;

};
layout(location = 0) out VertexOutput Output;

void main()
{
    Output.TexelSize = a_texelSize;
    int idx = indices[gl_VertexID];
    vec4 pos = vec4(pos2D[idx], 1.0);
    Output.TexCoords = tex[idx];
    Output.invTexelRatio = a_invTexelRatio;

    vec2 dx = vec2(a_texelSize.x, 0.0f) * a_step;
    vec2 dy = vec2(0.0f, a_texelSize.y) * a_step;

    Output.UV[0] = Output.TexCoords;

    //Sample all pixels within a 3x3 block
    Output.UV[1] = Output.TexCoords + dx;
    Output.UV[2] = Output.TexCoords - dx;
    Output.UV[3] = Output.TexCoords + dy;
    Output.UV[4] = Output.TexCoords - dy;
    Output.UV[5] = Output.TexCoords + dx + dy;
    Output.UV[6] = Output.TexCoords + dx - dy;
    Output.UV[7] = Output.TexCoords - dx + dy;
    Output.UV[8] = Output.TexCoords - dx - dy;

    gl_Position = vec4(pos2D[idx], 1.0);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 FragColor;

struct VertexOutput
{
    vec2 TexCoords;
    vec2 TexelSize;
    vec2 UV[9];
    float invTexelRatio;
};
layout(location = 0) in VertexOutput Input;

layout(binding = 1) uniform sampler2D u_Texture;

float ScreenDistance(vec2 v)
{
    v.y *= Input.invTexelRatio;
    return length(v) * length(v);
}

void BoundsCheck(inout vec2 xy, vec2 uv)
{
    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f)
        xy = vec2(1000.0f);
}

void main()
{
    vec4 pixel = texture(u_Texture, Input.UV[0]);

    for (int j = 1; j <= 8; j++)
    {
        // Sample neighbouring pixel and make sure it's
        // on the same side as us
        vec4 n = texture(u_Texture, Input.UV[j]);
        if (n.w != pixel.w)
            n.xyz = vec3(0.0f);

        n.xy += Input.UV[j] - Input.UV[0];

        // Invalidate out of bounds neighbours
        BoundsCheck(n.xy, Input.UV[j]);

        float dist = ScreenDistance(n.xy);
        pixel.z = min(dist, pixel.z);
    }

    //// Signed distance (squared)
    float dist = sqrt(pixel.z);
    float alpha = smoothstep(0.004f, 0.002f, dist);
    if (alpha == 0.0)
        discard;

    vec3 outlineColor = vec3(1.0f, 0.0f, 0.0f);
    pixel = vec4(outlineColor, alpha);
    FragColor = pixel;
}