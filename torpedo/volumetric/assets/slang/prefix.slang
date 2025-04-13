import splat;

[[vk::push_constant]]
uniform PointCloud pc;

StructuredBuffer<Splat> splats;             // set 0, binding 0
RWStructuredBuffer<uint32_t> prefixOffsets; // set 0, binding 1
RWStructuredBuffer<uint32_t> tilesRendered; // set 0, binding 2

// Computes EXCLUSIVE prefix sum over full list of touched tile counts by Gaussians, based on:
// https://research.nvidia.com/publication/2016-03_single-pass-parallel-prefix-scan-decoupled-look-back
// https://github.com/linebender/vello/blob/custom-hal-archive-with-shaders/tests/shader/prefix.comp
// https://shi-yan.github.io/webgpuunleashed/Compute/prefix_sum.html
// The total number of touched tiles is written to `tilesRendered` for CPU readback.

[shader("compute")]
[numthreads(LINEAR_WORKGROUP_SIZE, 1, 1)]
void main(
    uint3 globalInvocationID : SV_DispatchThreadID,
    uint3 localInvocationID  : SV_GroupThreadID,
    uint3 groupID            : SV_GroupID,
) {
    let idx = globalInvocationID.x;
    if (idx >= pc.count) return;

    // TODO: replace with a better prefix scan algorithm
    prefixOffsets[idx] = 0;
    for (uint i = 0; i < idx; ++i) {
        prefixOffsets[idx] += splats[i].tiles;
    }

    // Write the total sum of touched tiles to `tilesRendered` for CPU readback
    if (idx == pc.count - 1) {
        tilesRendered[0] = prefixOffsets[idx] + splats[idx].tiles;
    }
}