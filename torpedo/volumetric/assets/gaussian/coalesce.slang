import splat;

[[vk::push_constant]]
uniform RasterInfo info;

[[vk::binding(7)]]
StructuredBuffer<uint64_t> keys;

[[vk::binding(8)]]
StructuredBuffer<uint> vals;

[[vk::binding(9)]]
StructuredBuffer<uint> blockSums;

[[vk::binding(10)]]
StructuredBuffer<uint> localSums;

[[vk::binding(11)]]
RWStructuredBuffer<uint64_t> sortedKeys;

[[vk::binding(12)]]
RWStructuredBuffer<uint> splatIndices;

groupshared uint globalPrefixes[4]; // global exclusive prefix of each radix combination

[shader("compute")]
[numthreads(WORKGROUP_SIZE / 2, 1, 1)] // each thread (invocation) processes 2 items
void main(uint3 localInvocationID : SV_GroupThreadID, uint3 groupID : SV_GroupID) {
    let localID = localInvocationID.x; // [0, WORKGROUP_SIZE / 2 - 1]

    // The starting index in the global array for this partition
    let begin = groupID.x * WORKGROUP_SIZE;

    // Each thread grabs 2 adjacent items
    let aj = 2 * localID + 0;
    let bj = 2 * localID + 1;
    let kA = begin + aj < info.tilesRendered ? keys[begin + aj] : uint64_t::maxValue;
    let kB = begin + bj < info.tilesRendered ? keys[begin + bj] : uint64_t::maxValue;
    let vA = begin + aj < info.tilesRendered ? vals[begin + aj] : 0;
    let vB = begin + bj < info.tilesRendered ? vals[begin + bj] : 0;

    let keyA = uint((kA >> (2 * info.radixPass)) & 0x3ULL);
    let keyB = uint((kB >> (2 * info.radixPass)) & 0x3ULL);

    if (localID < 4) {
        // Initialize the global prefix for each radix combination
        if (localID == 0) globalPrefixes[0] = 0;
        if (localID == 1) globalPrefixes[1] = 0;
        if (localID == 2) globalPrefixes[2] = 0;
        if (localID == 3) globalPrefixes[3] = 0;

        // Compute global exclusive prefix of each radix combination
        for (uint i = 0; i < groupID.x; ++i) {
            if (localID == 0) globalPrefixes[0] += blockSums[i] & 0xFFu;
            if (localID == 1) globalPrefixes[1] += (blockSums[i] >> 8) & 0xFFu;
            if (localID == 2) globalPrefixes[2] += (blockSums[i] >> 16) & 0xFFu;
            if (localID == 3) globalPrefixes[3] += (blockSums[i] >> 24) & 0xFFu;
        }
    }

    // Global mapping
    GroupMemoryBarrierWithGroupSync();
    let idxA = globalPrefixes[keyA] + localSums[aj];
    let idxB = globalPrefixes[keyB] + localSums[bj];

    if (idxA < info.tilesRendered) {
        sortedKeys[idxA] = kA;
        splatIndices[idxA] = vA;
    }
    if (idxB < info.tilesRendered) {
        sortedKeys[idxB] = kB;
        splatIndices[idxB] = vB;
    }
}