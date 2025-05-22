import splat;

[[vk::push_constant]]
uniform RasterInfo info;

[[vk::binding(3)]]
RWStructuredBuffer<Splat> splats;

[[vk::binding(4)]]
RWStructuredBuffer<uint> tilesRendered; // readback by CPU

[[vk::binding(5)]]
RWStructuredBuffer<uint> partitionCount; // atomic counter for partitioning items between workgroups

[[vk::binding(6)]]
RWStructuredBuffer<uint> partitionDescriptors; // fence-free descriptor (flag + value)

groupshared uint tiles[WORKGROUP_SIZE]; // each workgroup loads global tile values into shared memory for faster access
groupshared uint partition; // which part of the global array this workgroup is responsible for
groupshared uint value; // partition descriptor from other workgroups are read into this variable

#define NUM_BANKS 16 // half the SIMD width
#define LOG_NUM_BANKS 4
#define CONFLICT_FREE_OFFSET(n)((n) >> NUM_BANKS + (n) >> (2 * LOG_NUM_BANKS))

static const uint FLAG_X = 0u; // invalid
static const uint FLAG_A = 1u; // aggregate available
static const uint FLAG_P = 2u; // prefix available

// For atomic read of partition descriptor
static const uint DUMMY = uint::maxValue;

// Performs in-place EXCLUSIVE prefix scan over the `tiles` touched by each Gaussian in the `splats` buffer.
// The total number of touched tiles (global reduction) is written to `tilesRendered` for CPU readback.
// Reduce-then-scan without bank conflicts: https://www.eecs.umich.edu/courses/eecs570/hw/parprefix.pdf
// Decoupled lookback: https://research.nvidia.com/sites/default/files/pubs/2016-03_Single-pass-Parallel-Prefix/nvr-2016-002.pdf

[shader("compute")]
[numthreads(WORKGROUP_SIZE / 2, 1, 1)] // each thread (invocation) processes 2 items
void main(uint3 localInvocationID : SV_GroupThreadID) {
    let localID = localInvocationID.x; // [0, WORKGROUP_SIZE / 2 - 1]

    // As described in section 4.4 of "Single-pass Parallel Prefix Scan with Decoupled Look-back",
    // workgroups are not necessarily scheduled in order. An atomic counter is therefore used to 
    // assign monotonically increasing partition IDs to each workgroup. This helps avoid deadlocks 
    // where a workgroup may spin waiting on another workgroup that has not yet been scheduled.
    if (localID == 0) {
        InterlockedAdd(partitionCount[0], 1, partition);
        InterlockedExchange(partitionDescriptors[partition], 0u); // intialize partition descriptor
    }
    GroupMemoryBarrierWithGroupSync(); // wait until thread 0 has obtained the partition for this workgroup

    // The starting index in the global array for this partition
    let begin = partition * WORKGROUP_SIZE;

    // Load data into shared memory, each thread grabs 2 items and operates at conflict-free offsets
    let aj = localID;
    let bj = localID + WORKGROUP_SIZE / 2;
    let bankOffsetA = CONFLICT_FREE_OFFSET(aj);
    let bankOffsetB = CONFLICT_FREE_OFFSET(bj);
    tiles[aj + bankOffsetA] = begin + aj < info.pointCount ? splats[begin + aj].tiles : 0;
    tiles[bj + bankOffsetB] = begin + bj < info.pointCount ? splats[begin + bj].tiles : 0;

    // Loop log2(n) levels for upsweep phase
    uint offset = 1;
    for (uint d = WORKGROUP_SIZE >> 1; d > 0; d >>= 1) {
        // Ensure all values at the previous level have been computed and visible
        GroupMemoryBarrierWithGroupSync();

        // Reduction step, Algorithm 3 in "Parallel Prefix Sum (Scan) with CUDA"
        if (localID < d) {
            var ai = offset * (2 * localID + 1) - 1;
            var bi = offset * (2 * localID + 2) - 1;
            ai += CONFLICT_FREE_OFFSET(ai);
            bi += CONFLICT_FREE_OFFSET(bi);

            tiles[bi] += tiles[ai];
        }
        offset *= 2;
    }

    // Update this workgroup's partition descriptor to aggregate-available state
    // Step 3 in "Single-pass Parallel Prefix Scan with Decoupled Look-back"
    let aggregate = tiles[WORKGROUP_SIZE - 1 + CONFLICT_FREE_OFFSET(WORKGROUP_SIZE - 1)]; // only valid for thread 0
    if (localID == 0 && partition > 0) {
        let a = (FLAG_A << 30) | aggregate;
        InterlockedExchange(partitionDescriptors[partition], a);
    } else if (localID == 0) {
        // The first partition updates to prefix-available state using the aggregate
        let p = (FLAG_P << 30) | aggregate;
        InterlockedExchange(partitionDescriptors[partition], p);
    }

    // Decoupled lookback: determine this workgroup's exclusive prefix
    // Step 4 in "Single-pass Parallel Prefix Scan with Decoupled Look-back"
    var exclusivePrefix = 0u;
    if (partition > 0) {
        // Pooling starts from the immediate preceding partition
        var lookbackPartition = partition - 1;
        while (true) {
            // TODO: have an entire thread warp inspecting predecessors in parallel (parallelized look-back)
            if (localID == 0) InterlockedCompareExchange(partitionDescriptors[lookbackPartition], DUMMY, DUMMY, value);
            GroupMemoryBarrierWithGroupSync(); // wait for thread 0 to atomically obtain the descriptor value

            let flag = value >> 30;
            if (flag != FLAG_X) {
                // Accumulate the aggregate/inclusive prefix
                exclusivePrefix += uint(value & 0x3FFFFFFFu);
                if (flag == FLAG_A) --lookbackPartition;
                else break; // FLAG_P
            }
            // FLAG_X, spin
            GroupMemoryBarrierWithGroupSync();
        }
    }

    // Compute and record the partition-wide inclusive prefixes
    // Step 5 in "Single-pass Parallel Prefix Scan with Decoupled Look-back"
    if (localID == 0 && partition > 0) {
        let inclusivePrefix = aggregate + exclusivePrefix;
        let p = (FLAG_P << 30) | inclusivePrefix;
        InterlockedExchange(partitionDescriptors[partition], p);
    }

    // Set the last tile to this workgroup's exclusive prefix for the downsweep phase
    if (localID == 0) {
        tiles[WORKGROUP_SIZE - 1 + CONFLICT_FREE_OFFSET(WORKGROUP_SIZE - 1)] = exclusivePrefix;

        // The last partition writes the total sum of touched tiles for CPU readback and resets the atomic counter
        let workgroupCount = (info.pointCount + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE;
        if (partition == workgroupCount - 1) {
            tilesRendered[0] = aggregate + exclusivePrefix;
            partitionCount[0] = 0u;
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

            let t = tiles[ai];
            tiles[ai] = tiles[bi];
            tiles[bi] += t;
        }
    }

    // Make sure all shared memory writes are visible before writing to global memory
    GroupMemoryBarrierWithGroupSync();
    if (begin + aj < info.pointCount) splats[begin + aj].tiles = tiles[aj + bankOffsetA];
    if (begin + bj < info.pointCount) splats[begin + bj].tiles = tiles[bj + bankOffsetB];
}