import splat;

[[vk::push_constant]]
uniform RasterInfo info;

[[vk::binding(7)]]
StructuredBuffer<uint64_t> splatKeys;

[[vk::binding(9)]]
RWStructuredBuffer<uint> histograms; // PLACES_PER_KEY histograms of size BINS_PER_PLACE

static const uint K = 64; // key size in bits
static const uint PLACES_PER_KEY = K / 8; // number of digit places in K-bit keys
static const uint BINS_PER_PLACE = 256;   // number of bins per digit place, must be <= WORKGROUP_SIZE

#ifndef TILE_SIZE // how many items to process in a workgroup, must be >= WORKGROUP_SIZE, and divisible by 32
#define TILE_SIZE 1536 // staying within 48kB shared memory limit, given SUBGROUP_SIZE = 32
#endif
static const uint ITEMS_PER_THREAD = TILE_SIZE / WORKGROUP_SIZE;

struct Histogram { uint count[BINS_PER_PLACE]; }
groupshared Histogram hists[PLACES_PER_KEY];

[shader("compute")]
[numthreads(WORKGROUP_SIZE, 1, 1)] // launching tilesRendered / TILE_SIZE workgroups
void main(uint3 localInvocationID : SV_GroupThreadID, uint3 groupID : SV_GroupID) {
    let localID = localInvocationID.x; // [0, WORKGROUP_SIZE - 1]
    let begin = groupID.x * TILE_SIZE; // note that we no longer process WORKGROUP_SIZE items

    // Initialize private histograms
    for (uint p = 0; p < PLACES_PER_KEY; p++) if (localID < BINS_PER_PLACE) hists[p].count[localID] = 0;
    GroupMemoryBarrierWithGroupSync();

    // Consume keys and update private histograms
    for (uint i = 0; i < ITEMS_PER_THREAD; ++i) {
        let idx = begin + localID + i * WORKGROUP_SIZE;
        if (idx >= info.tilesRendered) break;

        let key = splatKeys[idx];
        for (uint p = 0; p < PLACES_PER_KEY; p++) {
            let digit = uint((key >> (p * 8)) & 0xFFULL);
            InterlockedAdd(hists[p].count[digit], 1u);
        }
    }
    GroupMemoryBarrierWithGroupSync();

    // Accumulate the results into the global counts
    for (uint p = 0; p < PLACES_PER_KEY; p++) {
        let idx = p * BINS_PER_PLACE + localID;
        if (localID < BINS_PER_PLACE) InterlockedAdd(histograms[idx], hists[p].count[localID]);
    }
}