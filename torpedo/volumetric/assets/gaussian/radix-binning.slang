import splat;

[[vk::push_constant]]
uniform RasterInfo info;

[[vk::binding(7)]]
RWStructuredBuffer<uint64_t> splatKeys;

[[vk::binding(8)]]
RWStructuredBuffer<uint> splatIndices;

[[vk::binding(9)]]
StructuredBuffer<uint> histograms; // PLACES_PER_KEY histograms of size BINS_PER_PLACE

[[vk::binding(10)]]
RWStructuredBuffer<uint> tileCount; // atomic counter for tile partition

[[vk::binding(11)]]
RWStructuredBuffer<uint> tileDescriptors; // each worgroup maintains BINS_PER_PLACE descriptors for each digit

[[vk::binding(12)]]
RWStructuredBuffer<uint64_t> tempKeys;

[[vk::binding(13)]]
RWStructuredBuffer<uint> tempVals;

#ifndef SUBGROUP_SIZE // i.e. warp in CUDA or wave in HLSL
#define SUBGROUP_SIZE 32 // default to NVIDIA warp size
#endif

#ifndef TILE_SIZE // how many items to process in a workgroup, must be >= WORKGROUP_SIZE, and divisible by 32
#define TILE_SIZE 1536 // staying within 48kB shared memory limit, given SUBGROUP_SIZE = 32
#endif

static const uint WARPS_PER_TILE = WORKGROUP_SIZE / SUBGROUP_SIZE; // launching WORKGROUP_SIZE local threads
static const uint ITEMS_PER_WARP = TILE_SIZE / WARPS_PER_TILE;
static const uint BINS_PER_PLACE = 256; // number of bins per digit place, must be <= WORKGROUP_SIZE

groupshared uint tileID; // which tile in the global array this workgroup is responsible for
groupshared uint counts[WARPS_PER_TILE * BINS_PER_PLACE]; // each warp maintains a count for each digit

groupshared uint64_t loadedKeys[TILE_SIZE];
groupshared uint64_t sortedKeys[TILE_SIZE];
groupshared uint vals[TILE_SIZE];

groupshared uint localPrefixes[TILE_SIZE];
groupshared uint globalOffsets[BINS_PER_PLACE];
groupshared uint tileDigitSums[BINS_PER_PLACE];

static const uint FLAG_X = 0u; // invalid
static const uint FLAG_A = 1u; // aggregate available
static const uint FLAG_P = 2u; // prefix available

uint64_t getKey(uint idx, uint sortIter) {
    return (sortIter & 1u) == 0 ? splatKeys[idx] : tempKeys[idx];
}

void setKey(uint idx, uint64_t key, uint sortIter) {
    if ((sortIter & 1u) == 0) {
        tempKeys[idx] = key;
    } else {
        splatKeys[idx] = key;
    }
}

uint getVal(uint idx, uint sortIter) { 
    return (sortIter & 1u) == 0 ? splatIndices[idx] : tempVals[idx];
}

void setVal(uint idx, uint val, uint sortIter) {
    if ((sortIter & 1u) == 0) {
        tempVals[idx] = val;
    } else {
        splatIndices[idx] = val;
    }
}

int findMSB(uint4 mask) {
    if (mask.w != 0) return int(96 + firstbithigh(mask.w));
    if (mask.z != 0) return int(64 + firstbithigh(mask.z));
    if (mask.y != 0) return int(32 + firstbithigh(mask.y));
    if (mask.x != 0) return int(firstbithigh(mask.x));
    return -1; // no bits set
}

[shader("compute")]
[numthreads(WORKGROUP_SIZE, 1, 1)] // launching tilesRendered / TILE_SIZE workgroups
void main(uint3 localInvocationID : SV_GroupThreadID) {
    // Get the tile ID for this workgroup
    let localID = localInvocationID.x;
    if (localID == 0) InterlockedAdd(tileCount[0], 1, tileID);
    GroupMemoryBarrierWithGroupSync();

    // Reset descriptors and initialize shared arrays for this tile
    if (localID < BINS_PER_PLACE) {
        let idx = tileID * BINS_PER_PLACE + localID;
        InterlockedExchange(tileDescriptors[idx], 0u);
        globalOffsets[localID] = 0u;
        tileDigitSums[localID] = 0u;
    }

    let begin = tileID * TILE_SIZE; // from which index in the global array to start reading this workgroup's elements
    let shift = 8 * info.sortIteration; // how many bits to right shift the key to get the current digit place

    let warpID = localID / SUBGROUP_SIZE; // tile-wide warp ID [0, WARPS_PER_TILE - 1]
    let laneID = WaveGetLaneIndex(); // warp-wide thread ID [0, SUBGROUP_SIZE - 1]

    // Load input elements from global memory
    let batchBegin = warpID * ITEMS_PER_WARP;
    for (uint i = 0; i < ITEMS_PER_WARP; i += SUBGROUP_SIZE) {
        let idx = batchBegin + i + laneID;
        loadedKeys[idx] = (begin + idx < info.tilesRendered) ? getKey(begin + idx, info.sortIteration) : uint64_t::maxValue;
    }

    // Initialize warp-wide histograms
    let countBegin = warpID * 256;
    for (uint i = 0; i < 256; i += SUBGROUP_SIZE) {
        let idx = countBegin + i + laneID;
        counts[idx] = 0u;
    }

    // Perform warp-wide key ranking in batches using WLMS
    for (uint i = 0; i < ITEMS_PER_WARP; i += SUBGROUP_SIZE) {
        let idx = batchBegin + i + laneID;
        let key = uint((loadedKeys[idx] >> shift) & 0xFFULL);

        let matchMask = WaveMatch(key);
        let prefixSum = WaveMultiPrefixSum(1u, matchMask);
        localPrefixes[idx] = prefixSum + counts[countBegin + key];

        let leadID = uint(findMSB(matchMask));
        if (laneID == leadID) counts[countBegin + key] += prefixSum + 1u;
    }
    // Wait until all warps have finished processing their batches
    // and the warp-wide digit counts are ready
    GroupMemoryBarrierWithGroupSync();

    // Perform exclusive prefix sum across per-warp digit counts,
    // using up to BINS_PER_PLACE threads for each digit
    for (uint i = 0; i < WARPS_PER_TILE; ++i) {
        let idx = i * WARPS_PER_TILE + localID;
        if (localID < BINS_PER_PLACE) {
            let t = counts[idx];
            counts[idx] = tileDigitSums[localID];
            tileDigitSums[localID] += t;
        }
    }

    // Update to FLAG_A status
    let descriptorBegin = tileID * BINS_PER_PLACE;
    if (localID < BINS_PER_PLACE && tileID > 0) {
        let a = (FLAG_A << 30) | tileDigitSums[localID];
        InterlockedExchange(tileDescriptors[descriptorBegin + localID], a);
    } else if (localID < BINS_PER_PLACE) {
        let p = (FLAG_P << 30) | tileDigitSums[localID];
        InterlockedExchange(tileDescriptors[descriptorBegin + localID], p);
    }

    // Up to BINS_PER_PLACE threads participate in the chained lookback
    if (tileID > 0 && localID < BINS_PER_PLACE) {
        var lookbackTile = tileID - 1;
        while (true) {
            let descriptorID = lookbackTile * BINS_PER_PLACE + localID;
            uint value; // placeholder to read descriptor value
            InterlockedCompareExchange(tileDescriptors[descriptorID], uint::maxValue, uint::maxValue, value);

            let flag = value >> 30;
            if (flag != FLAG_X) {
                // Accumulate the aggregate/inclusive prefix
                globalOffsets[localID] += uint(value & 0x3FFFFFFFu);
                if (flag == FLAG_A) --lookbackTile;
                else break; // FLAG_P
            }
        }

        // Update to FLAG-P status
        let inclusivePrefix = tileDigitSums[localID] + globalOffsets[localID];
        let p = (FLAG_P << 30) | inclusivePrefix;
        InterlockedExchange(tileDescriptors[descriptorBegin + localID], p);
    }

    // Yet another prefix scan, this time across the local digit sums
    uint offset = 1;
    for (uint d = BINS_PER_PLACE >> 1; d > 0; d >>= 1) {
        GroupMemoryBarrierWithGroupSync();
        // Reduction step
        if (localID < d) {
            var ai = offset * (2 * localID + 1) - 1;
            var bi = offset * (2 * localID + 2) - 1;

            tileDigitSums[bi] += tileDigitSums[ai];
        }
        offset *= 2;
    }

    // Set the last value to 0
    if (localID == 0) tileDigitSums[BINS_PER_PLACE - 1] = 0;

    // Traverse down tree & build prefix scan
    for (uint d = 1; d < BINS_PER_PLACE; d *= 2) {
        offset >>= 1;
        GroupMemoryBarrierWithGroupSync();

        // Partial sum step
        if (localID < d) {
            var ai = offset * (2 * localID + 1) - 1;
            var bi = offset * (2 * localID + 2) - 1;

            let t = tileDigitSums[ai];
            tileDigitSums[ai] = tileDigitSums[bi];
            tileDigitSums[bi] += t;
        }
    }

    // Local shuffling of keys and values
    GroupMemoryBarrierWithGroupSync();
    for (uint i = 0; i < ITEMS_PER_WARP; i += SUBGROUP_SIZE) {
        let idx = batchBegin + i + laneID;
        let key = loadedKeys[idx];
        let val = begin + idx < info.tilesRendered ? splatIndices[begin + idx] : 0;

        let d = uint((key >> shift) & 0xFFULL);
        let shuffleID = localPrefixes[idx] + counts[countBegin + d] + tileDigitSums[d];
        sortedKeys[shuffleID] = key;
        vals[shuffleID] = val;
    }

    // Global mapping of sorted keys and values
    GroupMemoryBarrierWithGroupSync();
    let histogramBegin = info.sortIteration * BINS_PER_PLACE;
    for (uint i = 0; i < BINS_PER_PLACE; ++i) {
        let globalBegin = histograms[histogramBegin + i];

        let localBegin = tileDigitSums[i];
        let localEnd = (i == BINS_PER_PLACE - 1) ? TILE_SIZE : tileDigitSums[i + 1];
        for (uint j = localID; j < localEnd - localBegin; j += WORKGROUP_SIZE) {
            let mapID = globalBegin + globalOffsets[i] + j;
            if (mapID >= info.tilesRendered) break;
            setKey(mapID, sortedKeys[localBegin + j], info.sortIteration);
            setVal(mapID, vals[localBegin + j], info.sortIteration);
        }
    }

    // The last workgroup reset the atomic counter
    let workgroupCount = (info.tilesRendered + TILE_SIZE - 1) / TILE_SIZE;
    if (tileID == workgroupCount - 1 && localID == 0) tileCount[0] = 0;
}
