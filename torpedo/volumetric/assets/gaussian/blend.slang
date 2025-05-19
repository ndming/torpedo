import splat;

[[vk::binding(0)]]
WTexture2D outputImage;

[[vk::binding(3)]]
StructuredBuffer<Splat> splats;

[[vk::binding(8)]]
StructuredBuffer<uint> splatIndices;

[[vk::binding(18)]]
StructuredBuffer<uint2> ranges;

static const uint BLOCK_SIZE = BLOCK_X * BLOCK_Y;

groupshared uint indices[BLOCK_SIZE];
groupshared float2 imagePoints[BLOCK_SIZE];
groupshared float4 copacs[BLOCK_SIZE];
groupshared uint doneCount;

[shader("compute")]
[numthreads(BLOCK_X, BLOCK_Y, 1)]
void main(uint3 localInvocationID : SV_GroupThreadID, uint3 tileID : SV_GroupID) {
    // Thread ID in the workgroup, [0, BLOCK_SIZE - 1]
    let localID = localInvocationID.x + localInvocationID.y * BLOCK_X;

    // Image size
    uint2 imageSize; uint mipCount;
    outputImage.GetDimensions(0, imageSize.x, imageSize.y, mipCount);

    // Identify this thread's pixel coordinates and check if they are inside the image
    let pixel = uint2(tileID.x * BLOCK_X, tileID.y * BLOCK_Y) + localInvocationID.xy;
    let inside = pixel.x < imageSize.x && pixel.y < imageSize.y;

    // Done threads can help with fetching, but don't rasterize
    var done = !inside;

    // Find the range of Gaussian indices this tile is responsible for
    let grid = getComputeGrid(imageSize);
    let range = ranges[tileID.y * grid.x + tileID.x];

    // Rasterization is done in rounds of BLOCK_SIZE threads
    let rounds = (range.y - range.x + BLOCK_SIZE - 1) / BLOCK_SIZE;
    var remaining = range.y - range.x; // how many Gaussian left to rasterize

    // Initialize the done count
    if (localID == 0) doneCount = 0;
    GroupMemoryBarrierWithGroupSync();

    var T = 1.0;
    var color = float3(0.0, 0.0, 0.0);
    var incremented = false;
    for (uint i = 0; i < rounds; i++, remaining -= BLOCK_SIZE) {
        // Done threads increment the counter, but should only do so once
        if (done && !incremented) {
            InterlockedAdd(doneCount, 1u);
            incremented = true;
        }

        // End if the entire block votes that it is done rasterizing
        GroupMemoryBarrierWithGroupSync();
        if (doneCount >= BLOCK_SIZE) break;

        // Load BLOCK_SIZE splats into shared memory
        let progress = i * BLOCK_SIZE + localID;
        if (range.x + progress < range.y) {
            let splatIdx = splatIndices[range.x + progress];
            indices[localID] = splatIdx;
            imagePoints[localID] = splats[splatIdx].texel.xy;
            copacs[localID] = splats[splatIdx].copac;
        }
        GroupMemoryBarrierWithGroupSync();

        // Rasterize the Gaussians in the current round
        let limit = min(BLOCK_SIZE, remaining);
        for (uint j = 0; !done && j < limit; j++) {
            let imgPoint = imagePoints[j]; // the splat center in image space
            let conic = copacs[j].xyz;
            let opacity = copacs[j].w;

            // Resample using conic matrix
            let d = imgPoint - float2(pixel.x, pixel.y);
            let power = -0.5 * (conic.x * d.x * d.x + conic.z * d.y * d.y) - conic.y * d.x * d.y;
            if (power > 0.0) continue;

            // Eq. (2) from 3D Gaussian splatting paper
            let alpha = min(0.99, opacity * exp(power));
            if (alpha < 1.0 / 255.0) continue;

            // Stop if this thread has blended enough splats, but it can still help fetching Gaussians
            if (T * (1 - alpha) < 0.0001f) {
                done = true;
                continue;
            }

            // Eq. (3) from 3D Gaussian splatting paper
            color += splats[indices[j]].color * alpha * T;
            T *= (1.0 - alpha);
        }
    }

    // All threads that treat valid pixel write out their final color
    if (inside) outputImage.Store(pixel, float4(color, 1.0));
}