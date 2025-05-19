import splat;

[[vk::push_constant]]
uniform RasterInfo info;

[[vk::binding(10)]]
RWStructuredBuffer<uint64_t> globalPrefixes; // per-block sums of radix 3 - radix 2

[[vk::binding(15)]]
RWStructuredBuffer<uint> blockCount; // atomic counter for workgroup partition

[[vk::binding(16)]]
RWStructuredBuffer<uint64_t> blockDescriptors; // flag (2 bits) - radix 3 (31 bits) - radix 2 (31 bits)

[[vk::binding(17)]]
RWStructuredBuffer<uint> globalSums; // total count of radix 0, 1, and 2

groupshared uint sum2[WORKGROUP_SIZE];
groupshared uint sum3[WORKGROUP_SIZE];
groupshared uint partition; // which part of the global array this workgroup is resonsible for
groupshared uint64_t value; // descriptor values from other workgroups are read into this variable

#define NUM_BANKS 16 // half the SIMD width
#define LOG_NUM_BANKS 4
#define CONFLICT_FREE_OFFSET(n)((n) >> NUM_BANKS + (n) >> (2 * LOG_NUM_BANKS))

static const uint FLAG_X = 0u; // invalid
static const uint FLAG_A = 1u; // aggregate available
static const uint FLAG_P = 2u; // prefix available

// For atomic read of block descriptors
static const uint64_t DUMMY = uint64_t::maxValue;

// Scans radix 2 and 3, packed together in globalPrefixes
// 4-way radix sort: https://www.sci.utah.edu/~csilva/papers/cgf.pdf

[shader("compute")]
[numthreads(WORKGROUP_SIZE / 2, 1, 1)] // each thread (invocation) processes 2 items
void main(uint3 localInvocationID : SV_GroupThreadID) {
    let localID = localInvocationID.x; // [0, WORKGROUP_SIZE / 2 - 1]

    // Acquire partition and initialize block descriptors
    if (localID == 0) {
        InterlockedAdd(blockCount[0], 1, partition);
        InterlockedExchange(blockDescriptors[partition], 0);
    }
    GroupMemoryBarrierWithGroupSync();

    // The starting index in the global array for this partition
    let begin = partition * WORKGROUP_SIZE;

    // Each thread grabs 2 items and operates at conflict-free offsets
    let aj = localID;
    let bj = localID + WORKGROUP_SIZE / 2;
    let bankOffsetA = CONFLICT_FREE_OFFSET(aj);
    let bankOffsetB = CONFLICT_FREE_OFFSET(bj);

    // Load data into shared memory
    let n = (info.tilesRendered + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE;
    let localA = begin + aj < n ? globalPrefixes[begin + aj] : 0;
    let localB = begin + bj < n ? globalPrefixes[begin + bj] : 0;
    sum2[aj + bankOffsetA] = uint(localA & 0xFFFFFFFFULL);
    sum2[bj + bankOffsetB] = uint(localB & 0xFFFFFFFFULL);
    sum3[aj + bankOffsetA] = uint(localA >> 32);
    sum3[bj + bankOffsetB] = uint(localB >> 32);

    // Loop log2(n) levels for upsweep phase
    uint offset = 1;
    for (uint d = WORKGROUP_SIZE >> 1; d > 0; d >>= 1) {
        // Ensure all values at the previous level have been computed and visible
        GroupMemoryBarrierWithGroupSync();

        // Reduction step
        if (localID < d) {
            var ai = offset * (2 * localID + 1) - 1;
            var bi = offset * (2 * localID + 2) - 1;
            ai += CONFLICT_FREE_OFFSET(ai);
            bi += CONFLICT_FREE_OFFSET(bi);

            sum2[bi] += sum2[ai];
            sum3[bi] += sum3[ai];
        }
        offset *= 2;
    }

    // Update this workgroup's partition descriptor to aggregate-available state
    let a2 = sum2[WORKGROUP_SIZE - 1 + CONFLICT_FREE_OFFSET(WORKGROUP_SIZE - 1)]; // only valid for thread 0
    let a3 = sum3[WORKGROUP_SIZE - 1 + CONFLICT_FREE_OFFSET(WORKGROUP_SIZE - 1)]; // only valid for thread 0
    if (localID == 0 && partition > 0) {
        let a = (uint64_t(FLAG_A) << 62) | (uint64_t(a3) << 31) | uint64_t(a2);
        InterlockedExchange(blockDescriptors[partition], a);
    } else if (localID == 0) {
        // The first partition updates to prefix-available state using the aggregate
        let p = (uint64_t(FLAG_P) << 62) | (uint64_t(a3) << 31) | uint64_t(a2);
        InterlockedExchange(blockDescriptors[partition], p);
    }

    // Decoupled lookback: determine this workgroup's exclusive prefix
    var exclusivePrefix2 = 0u;
    var exclusivePrefix3 = 0u;
    if (partition > 0) {
        // Pooling starts from the immediate preceding partition
        var lookbackPartition = partition - 1;
        while (true) {
            // TODO: have an entire SIMD workgroups inspecting predecessors in parallel (parallelized look-back)
            if (localID == 0) InterlockedCompareExchange(blockDescriptors[lookbackPartition], DUMMY, DUMMY, value);
            GroupMemoryBarrierWithGroupSync(); // wait for thread 0 to atomically obtain the descriptor value

            let flag = uint(value >> 62);
            if (flag != FLAG_X) {
                // Accumulate the aggregate/inclusive prefix
                exclusivePrefix2 += uint(value & 0x7FFFFFFFULL);
                exclusivePrefix3 += uint((value >> 31) & 0x7FFFFFFFULL);
                if (flag == FLAG_A) --lookbackPartition;
                else break; // FLAG_P
            }
            // FLAG_X, spin
            GroupMemoryBarrierWithGroupSync();
        }
    }

    // Compute and record block-wide inclusive prefixes
    let p3 = exclusivePrefix3 + a3; // only valid for thread 0
    let p2 = exclusivePrefix2 + a2; // only valid for thread 0
    if (localID == 0 && partition > 0) {
        let p = (uint64_t(FLAG_P) << 62) | (uint64_t(p3) << 31) | uint64_t(p2);
        InterlockedExchange(blockDescriptors[partition], p);
    }

    // Set the last sum to this workgroup's exclusive prefix for the downsweep phase
    if (localID == 0) {
        sum2[WORKGROUP_SIZE - 1 + CONFLICT_FREE_OFFSET(WORKGROUP_SIZE - 1)] = exclusivePrefix2;
        sum3[WORKGROUP_SIZE - 1 + CONFLICT_FREE_OFFSET(WORKGROUP_SIZE - 1)] = exclusivePrefix3;

        // The last partition writes the total sum of of radix 2 to the global sum buffer and reset the atomic counter
        let workgroupCount = (n + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE;
        if (partition == workgroupCount - 1) {
            globalSums[2] = p2;
            blockCount[0] = 0u;
        }
    }

    // Traverse down tree & build prefix scan
    for (uint d = 1; d < WORKGROUP_SIZE; d *= 2) {
        offset >>= 1;
        GroupMemoryBarrierWithGroupSync();

        // Partial sum step, Algorithm 4 in "Parallel Prefix Sum (Scan) with CUDA"
        if (localID < d) {
            var ai = offset * (2 * localID + 1) - 1;
            var bi = offset * (2 * localID + 2) - 1;
            ai += CONFLICT_FREE_OFFSET(ai);
            bi += CONFLICT_FREE_OFFSET(bi);

            let t2 = sum2[ai];
            sum2[ai] = sum2[bi];
            sum2[bi] += t2;

            let t3 = sum3[ai];
            sum3[ai] = sum3[bi];
            sum3[bi] += t3;
        }
    }

    // Make sure all shared memory writes are visible before writing to global memory
    GroupMemoryBarrierWithGroupSync();
    if (begin + aj < n) globalPrefixes[begin + aj] = (uint64_t(sum3[aj + bankOffsetA]) << 32) | uint64_t(sum2[aj + bankOffsetA]);
    if (begin + bj < n) globalPrefixes[begin + bj] = (uint64_t(sum3[bj + bankOffsetB]) << 32) | uint64_t(sum2[bj + bankOffsetB]);
}