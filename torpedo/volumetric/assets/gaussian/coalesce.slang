import splat;

[[vk::push_constant]]
uniform RasterInfo info;

[[vk::binding(6)]]
RWStructuredBuffer<uint64_t> keys;

[[vk::binding(7)]]
RWStructuredBuffer<uint> splatIndices;

[[vk::binding(8)]]
RWStructuredBuffer<uint> blockSums;

[shader("compute")]
[numthreads(WORKGROUP_SIZE / 2, 1, 1)] // each thread (invocation) processes 2 items
void main(uint3 localInvocationID : SV_GroupThreadID, uint3 groupID : SV_GroupID) {
    
}