import splat;

[[vk::push_constant]]
uniform RasterInfo info;

[[vk::binding(7)]]
RWStructuredBuffer<uint64_t> splatKeys;

[[vk::binding(8)]]
RWStructuredBuffer<uint> splatIndices;

[[vk::binding(12)]]
StructuredBuffer<uint> globalSums; // total count of radix 0, 1, and 2

[[vk::binding(13)]]
StructuredBuffer<uint> globalPrefixes;

[[vk::binding(14)]]
StructuredBuffer<uint64_t> tempKeys;

[[vk::binding(15)]]
StructuredBuffer<uint> tempVals;

groupshared uint sums[3];

[shader("compute")]
[numthreads(WORKGROUP_SIZE, 1, 1)]
void main(uint3 globalInvocationID : SV_DispatchThreadID, uint3 localInvocationID : SV_GroupThreadID) {
    let idx = globalInvocationID.x;
    if (idx >= info.tilesRendered) return;

    let k = tempKeys[idx];
    let v = tempVals[idx];

    let shift = 2 * info.radixPass;
    let key = uint((k >> shift) & 0x3ULL);

    if (localInvocationID.x == 0) {
        sums[0] = globalSums[0];
        sums[1] = globalSums[1];
        sums[2] = globalSums[2];
        sums[1] += sums[0];
        sums[2] += sums[1];
    }
    GroupMemoryBarrierWithGroupSync();

    let offset = key > 0 ? sums[key - 1] : 0u;
    let shuffleIdx = globalPrefixes[idx] + offset;

    splatKeys[shuffleIdx] = k;
    splatIndices[shuffleIdx] = v;
}