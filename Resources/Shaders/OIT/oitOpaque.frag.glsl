#include <Resources/Shaders/OIT/ShaderCommon.glsl>

layout(location = 0) in Interpolants IN;

layout(location = 0) out vec4 outColor;

void main()
{
  vec3 color = IN.color.rgb * goochLighting(IN.normal);

  outColor = vec4(color, 1.0f);
}