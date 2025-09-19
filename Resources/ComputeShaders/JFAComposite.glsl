#type compute
#version 450 core

// GLSL compute shaders are organized into workgroups.
layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Output: The final image with the outline applied.
layout (set = 0, binding = 0, rgba8) writeonly uniform  image2D outputTexture;

// Input: The final data texture from the last 'jfa_pass', which contains coordinates.
layout (set = 0, binding = 1, rgba32f) readonly uniform image2D jfaDataTexture;

// Input: The original scene texture to composite the outline over.
layout (set = 0, binding = 2, rgba8) readonly uniform image2D sceneTexture;


// --- Configuration for a true outline ---
// The distance from the object's edge where the center of the outline will be.
const float outline_distance = 2.5;
// The total width of the outline band.
const float outline_width    = 5.0;
// The softness/feathering of the outline's edges. Should be <= half of the width.
const float outline_softness = 2.5;
// The color of the outline.
const vec4 outline_color     = vec4(0.9, 0.1, 0.3, 1.0);

void main() {
    // `gl_GlobalInvocationID` is the GLSL equivalent of Metal's `[[thread_position_in_grid]]`.
    ivec2 gid = ivec2(gl_GlobalInvocationID.xy);

    // Get the dimensions of the output texture for bounds checking.
    ivec2 texSize = imageSize(outputTexture);

    // Bounds check
    if (gid.x >= texSize.x || gid.y >= texSize.y) {
        return;
    }

    // --- Outline Logic ---
    // Read the coordinate of the nearest seed for this pixel.
    vec2 nearest_seed_pos = imageLoad(jfaDataTexture, gid).xy;
    
    // If no valid seed, just write the original scene color and exit.
    if (nearest_seed_pos.x < 0.0) {
        vec4 scene_color = imageLoad(sceneTexture, gid);
        imageStore(outputTexture, gid, scene_color);
        return;
    }
    
    // Calculate the true distance from the current pixel to its nearest seed.
    float dist = distance(vec2(gid), nearest_seed_pos);

    // 1. Calculate how far the current pixel's distance is from the ideal outline distance.
    // This value is 0.0 right on the outline and gets larger as you move away.
    float dist_from_outline = abs(dist - outline_distance);
    
    // 2. Use an inverted smoothstep to create a smooth band.
    // The alpha will be 1.0 when dist_from_outline is 0, and 0.0 when it's
    // greater than half the outline's width.
    float half_width = outline_width / 2.0;
    
    // GLSL's `smoothstep(edge0, edge1, x)` is the same as Metal's.
    float outline_alpha = 1.0 - smoothstep(
        half_width - outline_softness,
        half_width + outline_softness,
        dist_from_outline
    );

    // --- Compositing Logic ---
    // Read the original color from the scene.
    vec4 scene_color = imageLoad(sceneTexture, gid);
    
    // Mix the scene color and outline color based on the calculated alpha.
    // GLSL's `mix()` is the equivalent of Metal's `mix()`.
    vec4 final_color = mix(scene_color, outline_color, outline_alpha);
    
    // Write the final composed color to the output texture.
    imageStore(outputTexture, gid, final_color);
}