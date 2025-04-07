import splat;

[[vk::push_constant]]
uniform PointCloud pc; // push constant

StructuredBuffer<RasterPoint> rasterPoints; // set 0, binding 0
RWStructuredBuffer<uint32_t> prefixOffsets; // set 0, binding 1

// Computes EXCLUSIVE prefix sum over full list of touched tile counts by Gaussians, based on:
// https://research.nvidia.com/publication/2016-03_single-pass-parallel-prefix-scan-decoupled-look-back
// https://github.com/linebender/vello/blob/custom-hal-archive-with-shaders/tests/shader/prefix.comp
// https://shi-yan.github.io/webgpuunleashed/Compute/prefix_sum.html

[shader("compute")]
[numthreads(256, 1, 1)]
void main(uint3 globalInvocationID : SV_DispatchThreadID) {

}