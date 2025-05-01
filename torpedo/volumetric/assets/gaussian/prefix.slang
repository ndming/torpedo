import splat;

[[vk::push_constant]]
uniform RasterInfo info;

[[vk::binding(3)]]
StructuredBuffer<Splat> splats;

[[vk::binding(4)]]
RWStructuredBuffer<uint32_t> prefixOffsets;

[[vk::binding(5)]]
RWStructuredBuffer<uint32_t> tilesRendered; // readback by CPU

// Computes EXCLUSIVE prefix sum over full list of touched tile counts by Gaussians.
// The total number of touched tiles is written to `tilesRendered` for CPU readback.
// See: https://www.eecs.umich.edu/courses/eecs570/hw/parprefix.pdf

[shader("compute")]
[numthreads(WORKGROUP_SIZE, 1, 1)]
void main(
    uint3 globalInvocationID : SV_DispatchThreadID,
    uint3 localInvocationID  : SV_GroupThreadID,
    uint3 groupID            : SV_GroupID,
) {
    let idx = globalInvocationID.x;
    if (idx >= info.pointCount) return;

    // TODO: replace with a better prefix scan algorithm
    prefixOffsets[idx] = 0;
    for (uint i = 0; i < idx; ++i) {
        prefixOffsets[idx] += splats[i].tiles;
    }

    // Write the total sum of touched tiles to `tilesRendered` for CPU readback
    if (idx == info.pointCount - 1) {
        tilesRendered[0] = prefixOffsets[idx] + splats[idx].tiles;
    }
}