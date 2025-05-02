import splat;

[[vk::push_constant]]
uniform RasterInfo info;

[[vk::binding(0)]]
WTexture2D outputImage;

[[vk::binding(3)]]
StructuredBuffer<Splat> splats;

[[vk::binding(4)]]
StructuredBuffer<uint> offsets;

[[vk::binding(6)]]
RWStructuredBuffer<uint64_t> keys;

[[vk::binding(7)]]
RWStructuredBuffer<uint32_t> vals;

// Based on: https://github.com/graphdeco-inria/gaussian-splatting
// Generates one key/value pair for all Gaussian/tile overlaps

[shader("compute")]
[numthreads(WORKGROUP_SIZE, 1, 1)]
void main(uint3 globalInvocationID : SV_DispatchThreadID) {
    // Each thread processes one Gaussian point
    let idx = globalInvocationID.x;
    if (idx >= info.pointCount) return;

    // Don't generate key/value pairs for invisible Gaussians
    let radius = splats[idx].texel.w;
    if (radius <= 0) return;

    let depth = splats[idx].texel.z;
    let imgPoint = splats[idx].texel.xy;

    // The image size in pixels
    uint2 imageSize; uint mipCount;
    outputImage.GetDimensions(0, imageSize.x, imageSize.y, mipCount);

    let grid = getComputeGrid(imageSize);
    uint2 rectMin, rectMax;
    getBoundingRect(imgPoint, radius, getComputeGrid(imageSize), rectMin, rectMax);

    // Unlike the original CUDA code, we perform exclusive prefix sum,
    // thus the correct offset directly locates at the Gaussian index.
    var offset = offsets[idx];

    for (uint y = rectMin.y; y < rectMax.y; y++) {
        for (uint x = rectMin.x; x < rectMax.x; x++) {
            // Compute the key: uint64_t(uint32_t(tile index)-uint32_t(depth))
            uint64_t key = y * grid.x + x;
            key <<= 32;
            key |= reinterpret<uint32_t>(depth);
            keys[offset] = key;
            vals[offset] = idx;
            ++offset;
        }
    }
}