import splat;

[[vk::push_constant]]
uniform PointCloud pc;

StructuredBuffer<Splat> splats;            // set 0, binding 0
StructuredBuffer<uint32_t> prefixOffsets;  // set 0, binding 1

RWStructuredBuffer<uint64_t> keys;  // set 0, binding 2
RWStructuredBuffer<uint32_t> vals;  // set 0, binding 3

// Based on: https://github.com/graphdeco-inria/gaussian-splatting
// Generates one key/value pair for all Gaussian/tile overlaps

[shader("compute")]
[numthreads(LINEAR_WORKGROUP_SIZE, 1, 1)]
void main(uint3 globalInvocationID : SV_DispatchThreadID) {
    // Each thread processes one Gaussian point
    let idx = globalInvocationID.x;
    if (idx >= pc.count) return;

    // Don't generate key/value pairs for invisible Gaussians
    let radius = splats[idx].texel.w;
    if (radius <= 0) return;

    let depth = splats[idx].texel.z;
    let imgPoint = splats[idx].texel.xy;

    let grid = getComputeGrid(pc.imgSize);
    uint2 rectMin, rectMax;
    getBoundingRect(imgPoint, radius, getComputeGrid(pc.imgSize), rectMin, rectMax);

    // Unlike the original CUDA code, we perform exclusive prefix sum,
    // thus the correct offset directly locates at the Gaussian index.
    var offset = prefixOffsets[idx];

    for (uint y = rectMin.y; y < rectMax.y; y++) {
        for (uint x = rectMin.x; x < rectMax.x; x++) {
            // Compute the key: uint64_t(uint32_t(tile index)-uint32_t(depth))
            uint64_t key = y * grid.x + x;
            key <<= 32;
            key |= reinterpret<uint32_t>(depth);
            keys[offset] = key;
            keys[offset] = idx;
            ++offset;
        }
    }
}