import splat;

[[vk::push_constant]]
uniform PointCloud pc; // push constant

StructuredBuffer<RasterPoint> rasterPoints; // set 0, binding 0
RWStructuredBuffer<uint32_t> numRendered;   // set 0, binding 1

// Computes the total number of touched tiles and write it `numRenderer` for CPU readback.
// This is used to determine the size of the sorted key/value pairs, based on:
// https://dournac.org/info/gpu_sum_reduction
// https://developer.download.nvidia.com/assets/cuda/files/reduction.pdf

[shader("compute")]
[numthreads(256, 1, 1)]
void main(uint3 globalInvocationID : SV_DispatchThreadID) {
    
}