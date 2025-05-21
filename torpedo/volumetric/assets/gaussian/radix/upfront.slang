import splat;

[[vk::push_constant]]
uniform RasterInfo info;

[[vk::binding(7)]]
StructuredBuffer<uint64_t> splatKeys;

[[vk::binding(9)]]
RWStructuredBuffer<uint> histograms; // P histograms of size B

static const uint K = 64;    // key size in bits
static const uint P = K / 8; // number of digit places
static const uint B = 256;   // number of bins per digit place, must be <= WORKGROUP_SIZE

static const uint TILE_SIZE = 2048; // how many items to process in a workgroup
static const uint ITEMS_PER_THREAD = TILE_SIZE / WORKGROUP_SIZE;

struct Histogram {
    uint count[B];
}

groupshared Histogram hists[P];

[shader("compute")]
[numthreads(WORKGROUP_SIZE, 1, 1)] // launching tilesRendered / TILE_SIZE workgroups
void main(uint3 localInvocationID : SV_GroupThreadID, uint3 groupID : SV_GroupID) {
    let localID = localInvocationID.x; // [0, WORKGROUP_SIZE - 1]
    let begin = groupID.x * TILE_SIZE; // note that we no longer process WORKGROUP_SIZE items

    // Initialize private histograms
    for (uint p = 0; p < P; p++) if (localID < B) hists[p].count[localID] = 0;
    GroupMemoryBarrierWithGroupSync();

    // Consume keys and update private histograms
    for (uint i = 0; i < ITEMS_PER_THREAD; ++i) {
        let idx = begin + localID + i * WORKGROUP_SIZE;
        if (idx >= info.tilesRendered) break;

        let key = splatKeys[idx];
        for (uint p = 0; p < P; p++) {
            let digit = uint((key >> (p * 8)) & 0xFFULL);
            InterlockedAdd(hists[p].count[digit], 1u);
        }
    }
    GroupMemoryBarrierWithGroupSync();

    // Accumulate the results into the global counts
    for (uint p = 0; p < P; p++) {
        let idx = p * B + localID;
        if (localID < B) InterlockedAdd(histograms[idx], hists[p].count[localID]);
    }
}