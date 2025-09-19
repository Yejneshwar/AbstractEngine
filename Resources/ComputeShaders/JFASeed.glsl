#type compute
#version 450 core

// GLSL compute shaders are organized into workgroups.
// This defines the size of a workgroup in 2D (8x8 threads).
// This value is often tuned for performance on the target GPU.
layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Input: Your RGBA8 "mask" where seeds are marked.
// This corresponds to Metal's `[[texture(0)]]`.
// The `rgba8` format qualifier specifies the image format.
layout (binding = 0, rgba8) uniform readonly image2D maskTexture;

// Output: The RGBA32Float texture that will store seed coordinates.
// This corresponds to Metal's `[[texture(1)]]`.
// `rgba32f` is used to ensure enough precision for coordinates.
layout (binding = 1, rgba32f) uniform writeonly image2D seedDataTexture;

void main() {
    // `gl_GlobalInvocationID` is the GLSL equivalent of Metal's `[[thread_position_in_grid]]`.
    // We only need the x and y components, and we cast it to ivec2 for use with image functions.
    ivec2 gid = ivec2(gl_GlobalInvocationID.xy);

    // Get the dimensions of the texture for bounds checking.
    ivec2 texSize = imageSize(maskTexture);

    // Bounds check to avoid writing outside the texture dimensions.
    if (gid.x >= texSize.x || gid.y >= texSize.y) {
        return;
    }

    // Read the color from the source mask using `imageLoad`.
    // This is the equivalent of `maskTexture.read(gid)`.
    vec4 maskColor = imageLoad(maskTexture, gid);

    // This is your seeding logic. Here, we assume any pixel with a
    // red component greater than 0 is a seed.
    if (maskColor.r > 0.0f) {
        // This is a seed. Write this pixel's coordinate to the data texture.
        // `imageStore` is the equivalent of `texture.write()`.
        imageStore(seedDataTexture, gid, vec4(float(gid.x), float(gid.y), 0.0, 1.0));
    } else {
        // This is not a seed. Write the "null" value.
        imageStore(seedDataTexture, gid, vec4(-1.0, -1.0, 0.0, 1.0));
    }
}