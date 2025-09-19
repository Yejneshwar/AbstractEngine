#type compute
#version 450

layout(binding = 0, rgba32f) readonly uniform image2D inTexture;
layout(binding = 1, rgba32f) writeonly uniform image2D outTexture;

layout(location = 2) uniform int step;

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

void main() {
    ivec2 gid = ivec2(gl_GlobalInvocationID.xy);
    ivec2 imgSize = imageSize(inTexture);

    if (gid.x >= imgSize.x || gid.y >= imgSize.y) {
        return;
    }

    vec2 closest_seed_pos = imageLoad(inTexture, gid).xy;
    float min_dist_sq = -1.0;

    if (closest_seed_pos.x >= 0.0) {
        vec2 diff = vec2(gid) - closest_seed_pos;
        min_dist_sq = dot(diff, diff);
    }

    for (int j = -1; j <= 1; ++j) {
        for (int i = -1; i <= 1; ++i) {
            ivec2 sample_pos = gid + ivec2(i, j) * step;

            if (sample_pos.x >= 0 && sample_pos.y >= 0 &&
                sample_pos.x < imgSize.x && sample_pos.y < imgSize.y) {

                vec2 neighbor_seed_pos = imageLoad(inTexture, sample_pos).xy;

                if (neighbor_seed_pos.x >= 0.0) {
                    vec2 diff = vec2(gid) - neighbor_seed_pos;
                    float dist_sq = dot(diff, diff);

                    if (min_dist_sq < 0.0 || dist_sq < min_dist_sq) {
                        min_dist_sq = dist_sq;
                        closest_seed_pos = neighbor_seed_pos;
                    }
                }
            }
        }
    }

    imageStore(outTexture, gid, vec4(closest_seed_pos, 0.0, 1.0));
}
