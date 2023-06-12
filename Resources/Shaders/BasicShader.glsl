#type vertex
#version 450 core
layout(location = 0) in vec3 aPos;

layout(set = 0, binding = 0) uniform UBO{
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

void main()
{
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(aPos, 1.0);
}

#type fragment
#version 450 core
layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 1) uniform UBO{
    vec3 triangleColor;
} ubo;

void main()
{
    FragColor = vec4(ubo.triangleColor, 1.0);
}