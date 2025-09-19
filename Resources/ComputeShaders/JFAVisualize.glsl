#type compute
#version 450 core

// A compute shader that runs once after the JFA loop is complete.
// It reads the final coordinate data and writes a visual color.
// This is the GLSL equivalent of the Metal 'jfa_finalize_to_image' kernel.

// Define the local workgroup size. This is set on the CPU side in Metal,
// but is defined within the shader in GLSL. A common size like 8x8 is a good start.
layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Input: The final data texture from the last JFA pass.
layout(binding = 0, rgba8) writeonly uniform image2D finalOutputImage;

// Output: The final image to be displayed on screen.
layout(binding = 1, rgba32f) readonly uniform image2D jfaDataTexture;

void main() {
    // Get the thread's position in the grid.
    ivec2 gid = ivec2(gl_GlobalInvocationID.xy);

    // Get image dimensions for the bounds check.
    ivec2 image_dims = imageSize(finalOutputImage);

    // Bounds check to avoid writing outside the texture.
    if (gid.x >= image_dims.x || gid.y >= image_dims.y) {
        return;
    }

    // Read the coordinate of the nearest seed from the JFA data.
    vec2 nearest_seed_pos = imageLoad(jfaDataTexture, gid).xy;
    vec4 final_color;

    // Check if a valid seed was found for this pixel.
    if (nearest_seed_pos.x >= 0.0) {
        // Calculate the distance from the current pixel to its nearest seed.
        float dist = distance(vec2(gid), nearest_seed_pos);
        
        // Normalize the distance to a [0, 1] range to create a grayscale value.
        float max_dist = 256.0; 
        float color_value = clamp(dist / max_dist, 0.0, 1.0);
        
        final_color = vec4(color_value, color_value, color_value, 1.0);
    } else {
        // If no seed was found (e.g., empty input), output black.
        final_color = vec4(0.0, 0.0, 0.0, 1.0);
    }
    
    // Write the final visual color to the output image texture.
    imageStore(finalOutputImage, gid, final_color);
}