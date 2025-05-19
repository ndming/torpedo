import splat;

[[vk::push_constant]]
uniform RasterInfo info;

[[vk::binding(7)]]
StructuredBuffer<uint64_t> splatKeys;

[[vk::binding(8)]]
StructuredBuffer<uint> splatIndices;

[[vk::binding(9)]]
RWStructuredBuffer<uint64_t> globalPrefixA; // per-block sums of radix 1 - radix 0

[[vk::binding(10)]]
RWStructuredBuffer<uint64_t> globalPrefixB; // per-block sums of radix 3 - radix 2

[[vk::binding(11)]]
RWStructuredBuffer<uint64_t> tempKeys;

[[vk::binding(12)]]
RWStructuredBuffer<uint> tempVals;

struct LocalOffset {
    uint data[WORKGROUP_SIZE];
}

groupshared LocalOffset offsets[4];

#define NUM_BANKS 16 // half the SIMD width
#define LOG_NUM_BANKS 4
#define CONFLICT_FREE_OFFSET(n)((n) >> NUM_BANKS + (n) >> (2 * LOG_NUM_BANKS))

// Performs local shuffling of each radix combination
// 4-way radix sort: https://www.sci.utah.edu/~csilva/papers/cgf.pdf

[shader("compute")]
[numthreads(WORKGROUP_SIZE / 2, 1, 1)] // each thread (invocation) processes 2 items
void main(uint3 localInvocationID : SV_GroupThreadID, uint3 groupID : SV_GroupID) {
    let localID = localInvocationID.x; // [0, WORKGROUP_SIZE / 2 - 1]

    // The starting index in the global array for this partition
    let begin = groupID.x * WORKGROUP_SIZE;

    // Each thread grabs 2 items and operates at conflict-free offsets
    let aj = localID;
    let bj = localID + WORKGROUP_SIZE / 2;
    let bankOffsetA = CONFLICT_FREE_OFFSET(aj);
    let bankOffsetB = CONFLICT_FREE_OFFSET(bj);

    // Get the key and value (splat index) for each item
    let kA = begin + aj < info.tilesRendered ? splatKeys[begin + aj] : uint64_t::maxValue;
    let kB = begin + bj < info.tilesRendered ? splatKeys[begin + bj] : uint64_t::maxValue;
    let vA = begin + aj < info.tilesRendered ? splatIndices[begin + aj] : 0;
    let vB = begin + bj < info.tilesRendered ? splatIndices[begin + bj] : 0;

    // Generate mask for each 2-bit radix
    let shift = 2 * info.radixPass;
    let keyA = uint((kA >> shift) & 0x3ULL);
    let keyB = uint((kB >> shift) & 0x3ULL);
    offsets[0].data[aj + bankOffsetA] = uint(keyA == 0); offsets[0].data[bj + bankOffsetB] = uint(keyB == 0);
    offsets[1].data[aj + bankOffsetA] = uint(keyA == 1); offsets[1].data[bj + bankOffsetB] = uint(keyB == 1);
    offsets[2].data[aj + bankOffsetA] = uint(keyA == 2); offsets[2].data[bj + bankOffsetB] = uint(keyB == 2);
    offsets[3].data[aj + bankOffsetA] = uint(keyA == 3); offsets[3].data[bj + bankOffsetB] = uint(keyB == 3);

    // Upsweep reduction
    var offset = 1u;
    for (uint d = WORKGROUP_SIZE >> 1; d > 0; d >>= 1) {
        // Ensure all values at the previous level have been computed and visible
        GroupMemoryBarrierWithGroupSync();

        if (localID < d) {
            var ai = offset * (2 * localID + 1) - 1;
            var bi = offset * (2 * localID + 2) - 1;
            ai += CONFLICT_FREE_OFFSET(ai);
            bi += CONFLICT_FREE_OFFSET(bi);

            offsets[0].data[bi] += offsets[0].data[ai];
            offsets[1].data[bi] += offsets[1].data[ai];
            offsets[2].data[bi] += offsets[2].data[ai];
            offsets[3].data[bi] += offsets[3].data[ai];
        }
        offset *= 2;
    }

    // Set the last element of each local offset array to their respective local prefix sum
    // These s* values only valid for thread 0
    let n = WORKGROUP_SIZE;
    let s0 = offsets[0].data[n - 1 + CONFLICT_FREE_OFFSET(n - 1)];
    let s1 = offsets[1].data[n - 1 + CONFLICT_FREE_OFFSET(n - 1)];
    let s2 = offsets[2].data[n - 1 + CONFLICT_FREE_OFFSET(n - 1)];
    let s3 = offsets[3].data[n - 1 + CONFLICT_FREE_OFFSET(n - 1)];
    if (localID == 0) {
        offsets[3].data[n - 1 + CONFLICT_FREE_OFFSET(n - 1)] = s0 + s1 + s2;
        offsets[2].data[n - 1 + CONFLICT_FREE_OFFSET(n - 1)] = s0 + s1;
        offsets[1].data[n - 1 + CONFLICT_FREE_OFFSET(n - 1)] = s0;
        offsets[0].data[n - 1 + CONFLICT_FREE_OFFSET(n - 1)] = 0u;
    }

    // Downsweep partial sum
    for (uint d = 1; d < WORKGROUP_SIZE; d *= 2) {
        offset >>= 1;
        GroupMemoryBarrierWithGroupSync();

        if (localID < d) {
            var ai = offset * (2 * localID + 1) - 1;
            var bi = offset * (2 * localID + 2) - 1;
            ai += CONFLICT_FREE_OFFSET(ai);
            bi += CONFLICT_FREE_OFFSET(bi);

            let t0 = offsets[0].data[ai];
            offsets[0].data[ai] = offsets[0].data[bi];
            offsets[0].data[bi] += t0;

            let t1 = offsets[1].data[ai];
            offsets[1].data[ai] = offsets[1].data[bi];
            offsets[1].data[bi] += t1;

            let t2 = offsets[2].data[ai];
            offsets[2].data[ai] = offsets[2].data[bi];
            offsets[2].data[bi] += t2;

            let t3 = offsets[3].data[ai];
            offsets[3].data[ai] = offsets[3].data[bi];
            offsets[3].data[bi] += t3;
        }
    }

    // Local shuffling of per-block radix combinations
    GroupMemoryBarrierWithGroupSync();
    let idxA = offsets[keyA].data[aj + bankOffsetA];
    let idxB = offsets[keyB].data[bj + bankOffsetB];

    if (begin + idxA < info.tilesRendered) {
        tempKeys[begin + idxA] = kA;
        tempVals[begin + idxA] = vA;
    }
    if (begin + idxB < info.tilesRendered) {
        tempKeys[begin + idxB] = kB;
        tempVals[begin + idxB] = vB;
    }

    // Thread 0 write per-block sums of each radix to globalPrefixA/B
    if (localID == 0) {
        let sA = (uint64_t(s1) << 32) | uint64_t(s0);
        let sB = (uint64_t(s3) << 32) | uint64_t(s2);

        globalPrefixA[groupID.x] = sA;
        globalPrefixB[groupID.x] = sB;
    }
}