import splat;

[[vk::push_constant]]
uniform RasterInfo info;

[[vk::binding(7)]]
RWStructuredBuffer<uint64_t> splatKeys;

[[vk::binding(8)]]
RWStructuredBuffer<uint> splatIndices;

[[vk::binding(9)]]
StructuredBuffer<uint64_t> globalPrefixA; // per-block prefixes of radix 1 - radix 0

[[vk::binding(10)]]
StructuredBuffer<uint64_t> globalPrefixB; // per-block prefixes of radix 3 - radix 2

[[vk::binding(11)]]
StructuredBuffer<uint64_t> tempKeys;

[[vk::binding(12)]]
StructuredBuffer<uint> tempVals;

[[vk::binding(17)]]
StructuredBuffer<uint> globalSums; // total count of radix 0, 1, and 2

groupshared uint offsets[3]; // load global sums and pre compute offsets for chunk 1, 2, and 3
groupshared uint beginID[3]; // the local thread ID of the first radix 1, 2, and 3

groupshared uint64_t keys[WORKGROUP_SIZE]; // for faster access
groupshared uint globalPrefixes[4]; // unpacked global prefixes

[shader("compute")]
[numthreads(WORKGROUP_SIZE, 1, 1)]
void main(uint3 localInvocationID : SV_GroupThreadID, uint3 groupID : SV_GroupID) {
    let localID = localInvocationID.x; // [0, WORKGROUP_SIZE - 1]
    let begin = groupID.x * WORKGROUP_SIZE;
    if (begin + localID >= info.tilesRendered) return;

    // Load global keys to shared memory
    keys[localID] = tempKeys[begin + localID];

    // Precompute offsets for chunks of radix 1, 2, and 3
    if (localID == 0) {
        offsets[0] = globalSums[0]; // offset for radix 1
        offsets[1] = globalSums[1];
        offsets[2] = globalSums[2];
        offsets[1] += offsets[0]; // offset for radix 2
        offsets[2] += offsets[1]; // offset for radix 3
    }

    // Unpack global prefixes for each radix
    if (localID == 0) {
        let pA = globalPrefixA[groupID.x];
        let pB = globalPrefixB[groupID.x];
        globalPrefixes[0] = uint(pA & 0xFFFFFFFFULL);
        globalPrefixes[1] = uint(pA >> 32);
        globalPrefixes[2] = uint(pB & 0xFFFFFFFFULL);
        globalPrefixes[3] = uint(pB >> 32);
    }
    GroupMemoryBarrierWithGroupSync();

    let shift = 2 * info.radixPass;
    let key = uint((keys[localID] >> shift) & 0x3ULL);

    // Find begin thread ID of radix 1, 2, and 3
    if (localID > 0) {
        let prevKey = uint((keys[localID - 1] >> shift) & 0x3ULL);
        if (key != prevKey) beginID[key - 1] = localID;
    }
    GroupMemoryBarrierWithGroupSync();

    let offset = key > 0 ? offsets[key - 1] + localID - beginID[key - 1] : localID;
    let mapIdx = globalPrefixes[key] + offset;

    splatKeys[mapIdx] = keys[localID];
    splatIndices[mapIdx] = tempVals[begin + localID];
}