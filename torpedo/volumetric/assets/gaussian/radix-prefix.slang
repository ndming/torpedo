[[vk::binding(9)]]
RWStructuredBuffer<uint> histograms; // PLACES_PER_KEY histograms of size BINS_PER_PLACE

static const uint BINS_PER_PLACE = 256; // number of bins per digit place
groupshared uint bins[BINS_PER_PLACE];  // each workgroup loads global count values into shared memory for faster access

#ifndef NUM_BANKS
#define NUM_BANKS 16 // half the subgroup size
#define LOG_NUM_BANKS 4
#endif
#define CONFLICT_FREE_OFFSET(n)((n) >> NUM_BANKS + (n) >> (2 * LOG_NUM_BANKS))

[shader("compute")]
[numthreads(BINS_PER_PLACE / 2, 1, 1)] // launching P workgroups for each digit place, where each thread processes two items
void main(uint3 localInvocationID : SV_GroupThreadID, uint3 groupID : SV_GroupID) {
    let localID = localInvocationID.x;
    let begin = groupID.x * BINS_PER_PLACE;

    let aj = localID;
    let bj = localID + BINS_PER_PLACE / 2;
    let bankOffsetA = CONFLICT_FREE_OFFSET(aj);
    let bankOffsetB = CONFLICT_FREE_OFFSET(bj);
    bins[aj + bankOffsetA] = histograms[begin + aj];
    bins[bj + bankOffsetB] = histograms[begin + bj];

    // Loop log2(n) levels for upsweep phase
    uint offset = 1;
    for (uint d = BINS_PER_PLACE >> 1; d > 0; d >>= 1) {
        // Ensure all values at the previous level have been computed and visible
        GroupMemoryBarrierWithGroupSync();
        // Reduction step
        if (localID < d) {
            var ai = offset * (2 * localID + 1) - 1;
            var bi = offset * (2 * localID + 2) - 1;
            ai += CONFLICT_FREE_OFFSET(ai);
            bi += CONFLICT_FREE_OFFSET(bi);

            bins[bi] += bins[ai];
        }
        offset *= 2;
    }

    // Set the last value to 0
    if (localID == 0) bins[BINS_PER_PLACE - 1 + CONFLICT_FREE_OFFSET(BINS_PER_PLACE - 1)] = 0;

    // Traverse down tree & build prefix scan
    for (uint d = 1; d < BINS_PER_PLACE; d *= 2) {
        offset >>= 1;
        GroupMemoryBarrierWithGroupSync();

        // Partial sum step
        if (localID < d) {
            var ai = offset * (2 * localID + 1) - 1;
            var bi = offset * (2 * localID + 2) - 1;
            ai += CONFLICT_FREE_OFFSET(ai);
            bi += CONFLICT_FREE_OFFSET(bi);

            let t = bins[ai];
            bins[ai] = bins[bi];
            bins[bi] += t;
        }
    }

    // Make sure all shared memory writes are visible before writing to global memory
    GroupMemoryBarrierWithGroupSync();
    histograms[begin + aj] = bins[aj + bankOffsetA];
    histograms[begin + bj] = bins[bj + bankOffsetB];
}