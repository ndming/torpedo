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
groupshared uint globalPrefixes[4]; // unpacked global prefixes

groupshared uint chunkBegin[4]; // the starting position of each chunk
groupshared uint chunkEnd[4];   // the exclusive ending position of each chunk

groupshared uint64_t keys[WORKGROUP_SIZE]; // for faster access

// Performs coalesced mapping from local sorted keys to global keys
// 4-way radix sort: https://www.sci.utah.edu/~csilva/papers/cgf.pdf

[shader("compute")]
[numthreads(WORKGROUP_SIZE, 1, 1)]
void main(uint3 localInvocationID : SV_GroupThreadID, uint3 groupID : SV_GroupID) {
    let localID = localInvocationID.x; // [0, WORKGROUP_SIZE - 1]
    let begin = groupID.x * WORKGROUP_SIZE;
    if (begin + localID >= info.tilesRendered) return;

    // Load global keys to shared memory
    keys[localID] = tempKeys[begin + localID];
    let shift = 2 * info.sortIteration;
    let key = uint((keys[localID] >> shift) & 0x3ULL);

    if (localID == 0) {
        // Precompute offsets for chunks of radix 1, 2, and 3
        offsets[0] = globalSums[0]; // offset for radix 1
        offsets[1] = globalSums[1];
        offsets[2] = globalSums[2];
        offsets[1] += offsets[0]; // offset for radix 2
        offsets[2] += offsets[1]; // offset for radix 3

        // Unpack global prefixes for each radix
        let pA = globalPrefixA[groupID.x];
        let pB = globalPrefixB[groupID.x];
        globalPrefixes[0] = uint(pA & 0xFFFFFFFFULL);
        globalPrefixes[1] = uint(pA >> 32);
        globalPrefixes[2] = uint(pB & 0xFFFFFFFFULL);
        globalPrefixes[3] = uint(pB >> 32);

        // Initialize chunk boundaries
        chunkBegin[0] = 0; chunkEnd[0] = 0;
        chunkBegin[1] = 0; chunkEnd[1] = 0;
        chunkBegin[2] = 0; chunkEnd[2] = 0;
        chunkBegin[3] = 0; chunkEnd[3] = 0;
    }
    GroupMemoryBarrierWithGroupSync();

    // The last thread in the workgroup is responsible for the last chunk
    if (localID == WORKGROUP_SIZE - 1 || begin + localID == info.tilesRendered - 1) {
        chunkEnd[key] = localID + 1;
    }
    GroupMemoryBarrierWithGroupSync();

    // Find boundaries of each chunk
    if (localID > 0) {
        let prevKey = uint((keys[localID - 1] >> shift) & 0x3ULL);
        if (key != prevKey) {
            chunkBegin[key] = localID;
            chunkEnd[prevKey] = localID;
        }
    }
    GroupMemoryBarrierWithGroupSync();

    // Copy chunk 0
    if (localID < chunkEnd[0] - chunkBegin[0]) {
        let mapIdx = globalPrefixes[0] + localID;
        splatKeys[mapIdx] = keys[chunkBegin[0] + localID];
        splatIndices[mapIdx] = tempVals[begin + chunkBegin[0] + localID];
    }

    // Copy chunk 1
    if (localID < chunkEnd[1] - chunkBegin[1]) {
        let mapIdx = globalPrefixes[1] + localID + offsets[0];
        splatKeys[mapIdx] = keys[chunkBegin[1] + localID];
        splatIndices[mapIdx] = tempVals[begin + chunkBegin[1] + localID];
    }

    // Copy chunk 2
    if (localID < chunkEnd[2] - chunkBegin[2]) {
        let mapIdx = globalPrefixes[2] + localID + offsets[1];
        splatKeys[mapIdx] = keys[chunkBegin[2] + localID];
        splatIndices[mapIdx] = tempVals[begin + chunkBegin[2] + localID];
    }

    // Copy chunk 3
    if (localID < chunkEnd[3] - chunkBegin[3]) {
        let mapIdx = globalPrefixes[3] + localID + offsets[2];
        splatKeys[mapIdx] = keys[chunkBegin[3] + localID];
        splatIndices[mapIdx] = tempVals[begin + chunkBegin[3] + localID];
    }
}