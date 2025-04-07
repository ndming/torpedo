import splat;

[[vk::push_constant]]
uniform PointCloud pc; // push constant
uniform Camera camera; // set 0, binding 0

StructuredBuffer<RasterPoint> rasterPoints; // set 0, binding 1
StructuredBuffer<uint32_t> prefixOffsets;   // set 0, binding 2

RWStructuredBuffer<uint64_t> unsortedKeys;  // set 0, binding 3
RWStructuredBuffer<uint32_t> unsortedIdxs;  // set 0, binding 4

// Based on: https://github.com/graphdeco-inria/gaussian-splatting
// Generates one key/value pair for all Gaussian/tile overlaps

[shader("compute")]
[numthreads(256, 1, 1)]
void main(uint3 globalInvocationID : SV_DispatchThreadID) {
    // Each thread processes one Gaussian point
    let idx = globalInvocationID.x;
    if (idx >= pc.count) return;

    // Don't generate key/value pairs for invisible Gaussians
    let radius = rasterPoints[idx].texel.w;
    if (radius <= 0) return;

    let depth = rasterPoints[idx].texel.z;
    let imgPoint = rasterPoints[idx].texel.xy;

    let grid = getComputeGrid(camera.imgW, camera.imgH);
    uint2 rectMin, rectMax;
    getBoundingRect(imgPoint, radius, getComputeGrid(camera.imgW, camera.imgH), rectMin, rectMax);

    // Unlike the original CUDA code, we perform exclusive prefix sum,
    // thus the correct offset directly locates at the Gaussian index.
    var offset = prefixOffsets[idx];

    for (uint y = rectMin.y; y < rectMax.y; y++) {
        for (uint x = rectMin.x; x < rectMax.x; x++) {
            // Compute the key: uint64_t(uint32_t(tile index)-uint32_t(depth))
            uint64_t key = y * grid.x + x;
            key <<= 32;
            key |= reinterpret<uint32_t>(depth);
            unsortedKeys[offset] = key;
            unsortedIdxs[offset] = idx;
            ++offset;
        }
    }
}