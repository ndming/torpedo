import splat;

[[vk::push_constant]]
uniform RasterInfo info;

[[vk::binding(7)]]
StructuredBuffer<uint64_t> keys;

[[vk::binding(8)]]
StructuredBuffer<uint> vals;

[[vk::binding(9)]]
StructuredBuffer<uint64_t> blockSums;

[[vk::binding(10)]]
StructuredBuffer<uint> localSums;

[[vk::binding(11)]]
RWStructuredBuffer<uint64_t> sortedKeys;

[[vk::binding(12)]]
RWStructuredBuffer<uint> splatIndices;

groupshared uint globalPrefixes[4]; // global exclusive prefix of each key value
groupshared uint globalSums[4];     // global reductions of each radix combination

[shader("compute")]
[numthreads(WORKGROUP_SIZE, 1, 1)]
void main(uint3 localInvocationID : SV_GroupThreadID, uint3 groupID : SV_GroupID) {
    let localID = localInvocationID.x; // [0, WORKGROUP_SIZE - 1]

    // The starting index in the global array for this partition
    let begin = groupID.x * WORKGROUP_SIZE;
    if (begin + localID >= info.tilesRendered) return;

    let k = keys[begin + localID];
    let v = vals[begin + localID];

    let shift = 2 * info.radixPass;
    let key = uint((k >> shift) & 0x3ULL);

    if (localID < 4) {
        // Initialize global reductions
        if (localID == 0) globalSums[0] = 0;
        if (localID == 1) globalSums[1] = 0;
        if (localID == 2) globalSums[2] = 0;
        if (localID == 3) globalSums[3] = 0;

        let workgroupCount = (info.tilesRendered + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE;
        for (uint i = 0; i < workgroupCount; ++i) {
            if (localID == 0) {
                if (i == groupID.x) globalPrefixes[0] = globalSums[0];
                globalSums[0] += uint(blockSums[i] & 0xFFFFULL);
            }
            if (localID == 1) {
                if (i == groupID.x) globalPrefixes[1] = globalSums[1];
                globalSums[1] += uint((blockSums[i] >> 16) & 0xFFFFULL);
            }
            if (localID == 2) {
                if (i == groupID.x) globalPrefixes[2] = globalSums[2];
                globalSums[2] += uint((blockSums[i] >> 32) & 0xFFFFULL);
            }
            if (localID == 3) {
                if (i == groupID.x) globalPrefixes[3] = globalSums[3];
                globalSums[3] += uint((blockSums[i] >> 48) & 0xFFFFULL);
            }
        }

        GroupMemoryBarrierWithGroupSync();
        if (localID == 1) globalPrefixes[1] += globalSums[0];
        if (localID == 2) globalPrefixes[2] += globalSums[0] + globalSums[1];
        if (localID == 3) globalPrefixes[3] += globalSums[0] + globalSums[1] + globalSums[2];
    }

    // Global mapping
    GroupMemoryBarrierWithGroupSync();
    let idx = globalPrefixes[key] + localSums[begin + localID];
    sortedKeys[idx] = k;
    splatIndices[idx] = v;
}