implementing splatting;

static const float NEAR_PLANE = 0.2;

public bool passNearCulling(float3 point, float4x4 viewMatrix, out float3 viewPos) {
    viewPos = mul(viewMatrix, float4(point, 1.0)).xyz;
    if (viewPos.z <= NEAR_PLANE) return false;
    return true;
}

public float ndc2pix(float v, uint size) {
    return ((v + 1.0) * size - 1.0) * 0.5;
}

/// Returns the screen-space starting tile index `rectMin` and ending tile index `rectMax` (exclusive) thats the image
/// `point` with `radius` in pixels overlaps with. Tiles are `BLOCK_X` x `BLOCK_Y` making up a `grid` covering the image.
public void getBoundingRect(float2 point, float radius, uint2 grid, out uint2 rectMin, out uint2 rectMax) {
    rectMin.x = min(grid.x, max(0, (int)((point.x - radius) / BLOCK_X)));
    rectMin.y = min(grid.y, max(0, (int)((point.y - radius) / BLOCK_Y)));
    rectMax.x = min(grid.x, max(0, (int)((point.x + radius + BLOCK_X - 1) / BLOCK_X)));
    rectMax.y = min(grid.y, max(0, (int)((point.y + radius + BLOCK_Y - 1) / BLOCK_Y)));
}
