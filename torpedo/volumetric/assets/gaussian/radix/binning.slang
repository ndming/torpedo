import splat;

[[vk::push_constant]]
uniform RasterInfo info;

[[vk::binding(7)]]
StructuredBuffer<uint64_t> splatKeys;

[[vk::binding(9)]]
StructuredBuffer<uint> histograms; // P histograms of size B

[[vk::binding(10)]]
RWStructuredBuffer<uint> tileCount; // atomic counter for tile partition

static const uint ITEMS_PER_THREAD = 16; // stay below the 48kB limit
static const uint TILE_SIZE = ITEMS_PER_THREAD * WORKGROUP_SIZE;

groupshared uint64_t keys[TILE_SIZE];
groupshared uint tileID; // which tile in the global array this workgroup is responsible for

groupshared uint histogram[256]; // tile-wide histogram for each digit

[shader("compute")]
[numthreads(WORKGROUP_SIZE, 1, 1)] // launching tilesRendered / TILE_SIZE workgroups
void main(uint3 localInvocationID : SV_GroupThreadID, uint3 groupID : SV_GroupID) {
    let localID = localInvocationID.x;
    let warpSize = WaveGetLaneCount();

    if (localID == 0) InterlockedAdd(tileCount[0], 1, tileID);
    GroupMemoryBarrierWithGroupSync();

    let begin = tileID * TILE_SIZE;
    let itemsPerWarp = TILE_SIZE / warpSize;
    let warpID = localID / warpSize;
    let laneID = WaveGetLaneIndex();

    // Load input elements from global memory
    for (uint i = 0; i < itemsPerWarp; i += warpSize) {
        let idx = warpID * itemsPerWarp + i + laneID;
        keys[idx] = (begin + idx < info.tilesRendered) ? splatKeys[begin + idx] : uint64_t::maxValue;
    }
    GroupMemoryBarrierWithWaveSync();

    // Perform warp-wide key ranking using WLMS
    let batchCount = itemsPerWarp / warpSize;
    let shift = 8 * info.radixPass;

}