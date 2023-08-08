#include <Resources/Shaders/GLBufferDeclarations.h>

struct Interpolants
{
  vec3  pos;     // World-space vertex position
  vec3  normal;  // World-space vertex normal
  vec4  color;   // Linear-space color
  float depth;   // Z coordinate after applying the view matrix (larger = further away)
};

// Gooch shading!
// Interpolates between white and a cooler color based on the angle
// between the normal and the light.
vec3 goochLighting(vec3 normal)
{
  // Light direction
  vec3 light = normalize(vec3(-1, 2, 1));
  // cos(theta), remapped into [0,1]
  float warmth = dot(normalize(normal), light) * 0.5 + 0.5;
  // Interpolate between warm and cool colors (alpha here will be ignored)
  return mix(vec3(0, 0.25, 0.75), vec3(1, 1, 1), warmth);
}

// Applies Gooch shading to a surface with color and alpha and returns
// an unpremultiplied RGBA color.
vec4 shading(const Interpolants its, const vec4 triangleColor)
{
  vec3 colorRGB = triangleColor.rgb * goochLighting(its.normal);

  // Calculate transparency in [alphaMin, alphaMin+alphaWidth]
  float alpha = clamp(ubo.alphaMin + triangleColor.a * ubo.alphaWidth, 0, 1);

  return vec4(colorRGB, alpha);
}

// Converts an unpremultiplied scalar from linear space to sRGB. Note that
// this does not match the standard behavior outside [0,1].
float unPremultLinearToSRGB(float c)
{
  if(c < 0.0031308f)
  {
    return c * 12.92f;
  }
  else
  {
    return (pow(c, 1.0f / 2.4f) * 1.055f) - 0.055f;
  }
}

// Converts an unpremultiplied RGB color from linear space to sRGB. Note that
// this does not match the standard behavior outside [0,1].
vec4 unPremultLinearToSRGB(vec4 c)
{
  c.r = unPremultLinearToSRGB(c.r);
  c.g = unPremultLinearToSRGB(c.g);
  c.b = unPremultLinearToSRGB(c.b);
  return c;
}

// Converts an unpremultiplied scalar from sRGB to linear space. Note that
// this does not match the standard behavior outside [0,1].
float unPremultSRGBToLinear(float c)
{
  if(c < 0.04045f)
  {
    return c / 12.92f;
  }
  else
  {
    return pow((c + 0.055f) / 1.055f, 2.4f);
  }
}

// Converts an unpremultiplied RGBA color from sRGB to linear space. Note that
// this does not match the standard behavior outside [0,1].
vec4 unPremultSRGBToLinear(vec4 c)
{
  c.r = unPremultSRGBToLinear(c.r);
  c.g = unPremultSRGBToLinear(c.g);
  c.b = unPremultSRGBToLinear(c.b);
  return c;
}

// Sets color to the result of blending color over baseColor.
// Color and baseColor are both premultiplied colors.
void doBlend(inout vec4 color, vec4 baseColor)
{
  color.rgb += (1 - color.a) * baseColor.rgb;
  color.a += (1 - color.a) * baseColor.a;
}

// Sets color to the result of blending color over fragment.
// Color and fragment are both premultiplied colors; fragment
// is an rgba8 sRGB unpremultiplied color packed in a 32-bit uint.
void doBlendPacked(inout vec4 color, uint fragment)
{
  vec4 unpackedColor = unpackUnorm4x8(fragment);
  // Convert from unpremultiplied sRGB to premultiplied alpha
  unpackedColor = unPremultSRGBToLinear(unpackedColor);
  unpackedColor.rgb *= unpackedColor.a;
  doBlend(color, unpackedColor);
}