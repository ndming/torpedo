import splat;

[[vk::binding(9)]]
StructuredBuffer<uint> histograms; // P histograms of size B

[[vk::binding(10)]]
RWStructuredBuffer<uint> tileCount; // atomic counter for tile partition

static const uint ITEMS_PER_THREAD = 32; // the paper uses between 21 and 46
static const uint TILE_SIZE = ITEMS_PER_THREAD * WORKGROUP_SIZE;

[SpecializationConstant]
const uint SUBGROUP_SIZE = 32; // will be set based on GPU capabilities