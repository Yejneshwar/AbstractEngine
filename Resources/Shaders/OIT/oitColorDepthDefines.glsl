// Includes defines used for color and depth pases, including a statement
// that forces early depth testing.

// Important!
//
// This forces the depth/stencil test to be run prior to executing the shader.
// Otherwise, shaders that make use of shader writes could have fragments that
// execute even though the depth/stencil pass fails - in this case, transparent
// surfaces behind opaque objects (that we don't want to include in OIT)
// See https://www.khronos.org/opengl/wiki/Early_Fragment_Test
// In addition to that, post_depth_coverage also makes it so that gl_SampleMaskIn[]
// reflects the sample mask after depth testing, instead of before. This fixes
// problems with transparent and opaque objects in MSAA, and implicitly enables
// early_fragment_tests.
// See https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_post_depth_coverage.txt
#extension GL_ARB_post_depth_coverage : enable
layout(post_depth_coverage) in;

// If OIT_COVERAGE_SHADING is used, then the a-buffer uses three components;
// otherwise, it uses two.
#if OIT_COVERAGE_SHADING
#define abufferType rgba32ui
#define storeMask gl_SampleMaskIn[0]
#else  // #if OIT_COVERAGE_SHADING
#define abufferType rg32ui
#define storeMask 0
#endif  // #if OIT_COVERAGE_SHADING

#if OIT_SAMPLE_SHADING && OIT != OIT_WEIGHTED
#define uimage2DUsed uimage2DArray
#define sampleID gl_SampleID
ivec3 coord = ivec3(gl_FragCoord.xy, gl_SampleID);
#else  // #if OIT_SAMPLE_SHADING && OIT != OIT_WEIGHTED
#define uimage2DUsed uimage2D
#define sampleID 0
ivec2 coord = ivec2(gl_FragCoord.xy);
#endif  // #if OIT_SAMPLE_SHADING && OIT != OIT_WEIGHTED