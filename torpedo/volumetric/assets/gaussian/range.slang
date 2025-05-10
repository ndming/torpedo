import splat;

[[vk::push_constant]]
uniform RasterInfo info;

[[vk::binding(7)]]
StructuredBuffer<uint64_t> splatKeys;

[[vk::binding(16)]]
RWStructuredBuffer<uint2> ranges;

// Based on: https://github.com/graphdeco-inria/gaussian-splatting
// Each thread checks keys to see if it is at the start/end of one tile's range in the full sorted list.
// If yes, write start/end of this tile to the `ranges` buffer.

[shader("compute")]
[numthreads(WORKGROUP_SIZE, 1, 1)]
void main(uint3 globalInvocationID : SV_DispatchThreadID) {
    let idx = globalInvocationID.x;
    if (idx >= info.tilesRendered) return;

    let currTile = uint(splatKeys[idx] >> 32);
    if (idx == 0) {
        ranges[currTile].x = 0;
    } else {
        let prevTile = uint(splatKeys[idx - 1] >> 32);
        if (currTile != prevTile) {
            ranges[prevTile].y = idx;
            ranges[currTile].x = idx;
        }
    }
    if (idx == info.tilesRendered - 1) {
        ranges[currTile].y = idx + 1;
    }
}