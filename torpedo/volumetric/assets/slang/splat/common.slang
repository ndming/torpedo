implementing splat;

static const float NEAR_CULL = 0.2;

public bool passNearCulling(float3 point, float4x4 viewMatrix, out float3 viewPos) {
    viewPos = mul(viewMatrix, float4(point, 1.0)).xyz;
    if (viewPos.z <= NEAR_CULL) return false;
    return true;
}

public float2 ndc2pix(float2 ndcPoint, uint2 imageSize) {
    // The orignial CUDA code follows the old convention of rounding continuous coordinates to the nearest integer
    // to find the pixel center, hence the mapping [-1, 1] to [-0.5, size - 0.5] in this following code
    // More reading: https://pbr-book.org/4ed/Sampling_and_Reconstruction/Sampling_Theory#UnderstandingPixels
    return ((ndcPoint + 1.0) * imageSize - 1.0) * 0.5;
}

/// Returns the screen-space starting tile index `rectMin` and ending tile index `rectMax` (exclusive) thats the image
/// `point` with `radius` in pixels overlaps with. Tiles are `BLOCK_X` x `BLOCK_Y` making up a `grid` covering the image.
public void getBoundingRect(float2 point, float radius, uint2 grid, out uint2 rectMin, out uint2 rectMax) {
    rectMin.x = min(grid.x, max(0, (int)((point.x - radius) / BLOCK_X)));
    rectMin.y = min(grid.y, max(0, (int)((point.y - radius) / BLOCK_Y)));
    rectMax.x = min(grid.x, max(0, (int)((point.x + radius + BLOCK_X - 1) / BLOCK_X)));
    rectMax.y = min(grid.y, max(0, (int)((point.y + radius + BLOCK_Y - 1) / BLOCK_Y)));
}

/// Returns the number of `BLOCK_X` x `BLOCK_Y` tiles for a `imageW` x `imageH` (pixels) image.
public uint2 getComputeGrid(uint2 imageSize) {
    return uint2((imageSize.x + BLOCK_X - 1) / BLOCK_X, (imageSize.y + BLOCK_Y - 1) / BLOCK_Y);
}

public float3 extractWorldPosition(float4x4 viewMatrix) {
    // The 4th column of the view matrix is -R^T*t
    let R = transpose((float3x3)viewMatrix);
    return -mul(R, viewMatrix[3].xyz);
}