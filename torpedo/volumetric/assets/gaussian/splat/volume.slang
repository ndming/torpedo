implementing splat;

public float2 ndc2pix(float2 ndcPoint, uint2 imageSize) {
    // The orignial CUDA code follows the old convention of rounding continuous coordinates to the nearest integer to
    // find the pixel center, hence the mapping [-1, 1] to [-0.5, size - 0.5] in this following code
    // More reading: https://pbr-book.org/4ed/Sampling_and_Reconstruction/Sampling_Theory#UnderstandingPixels
    return ((ndcPoint + 1.0) * imageSize - 1.0) * 0.5;
}

/// Returns the screen-space starting tile index `rectMin` and ending tile index `rectMax` (exclusive) thats the image
/// `point` with `radius` in pixels overlaps with. `grid` specifies the number of `BLOCK_X` x `BLOCK_Y` tiles.
public void getBoundingRect(float2 point, float radius, uint2 grid, out uint2 rectMin, out uint2 rectMax) {
    rectMin.x = min(grid.x, max(0, (int)((point.x - radius) / BLOCK_X)));
    rectMin.y = min(grid.y, max(0, (int)((point.y - radius) / BLOCK_Y)));
    rectMax.x = min(grid.x, max(0, (int)((point.x + radius + BLOCK_X - 1) / BLOCK_X)));
    rectMax.y = min(grid.y, max(0, (int)((point.y + radius + BLOCK_Y - 1) / BLOCK_Y)));
}

/// Converts a `quaternion` and `scale` factors to a 3D covariance matrix in world space
public float3x3 computeCovariance(float4 quaternion, float4 scale) {
    // Create a scale matrix, scale modifier is stored in the w component of the scale vector
    let S = float3x3(
        scale.x * scale.w, 0.0, 0.0,
        0.0, scale.y * scale.w, 0.0,
        0.0, 0.0, scale.z * scale.w,
    );

    // Unlike the original CUDA code, the w component is stored in the 4th position of float4
    float x = quaternion.x, y = quaternion.y, z = quaternion.z, w = quaternion.w;
    // Compute rotation matrix from quaternion
    let R = float3x3(
        1.0 - 2.0 * (y * y + z * z), 2.0 * (x * y - w * z), 2.0 * (x * z + w * y),
        2.0 * (x * y + w * z), 1.0 - 2.0 * (x * x + z * z), 2.0 * (y * z - w * x),
        2.0 * (x * z - w * y), 2.0 * (y * z + w * x), 1.0 - 2.0 * (x * x + y * y),
    );

    // In the original CUDA code, the covariance matrix is computed as sigma = S * R
    // since they use glm which encodes matrices in column-major order
    let sigma = mul(R, S); // * in Slang is always element-wise multiplication 
    return mul(sigma, transpose(sigma)); // R * S * S^T * R^T
}

/// Projects a 3D covariance matrix of a Gaussian centered at `mean` in world space to a 2D covariance in clip space
public float2x2 projectCovariance(float3x3 cov3D, float3 mean, float4x4 viewMatrix, float2 focal, float2 tanHalfFov) {
    let W = (float3x3)viewMatrix;
    let J = computeJacobian(mean, viewMatrix, focal, tanHalfFov);
    let cov2D = mul(J, mul(W, mul(cov3D, transpose(W))) * transpose(J)); // J * W * Sigma_3D * W^T * J^T
    // Note that the low-pass filtering is hoisted out of this function, call site should apply it
    return (float2x2)cov2D;
}

/// Computes the local affine approximation of a projective transformation on `mean`, see EWA Spatting equation 29 
float3x3 computeJacobian(float3 mean, float4x4 viewMatrix, float2 focal, float2 tanHalfFov) {
    // The Gaussian's center in camera space
    var t = mul(viewMatrix, float4(mean, 1.0)).xyz;

    // TODO: Understand this logic
    let limx = 1.3 * tanHalfFov.x;
    let limy = 1.3 * tanHalfFov.y;
    let txtz = t.x / t.z;
    let tytz = t.y / t.z;
    t.x = min(limx, max(-limx, txtz)) * t.z;
    t.y = min(limy, max(-limy, tytz)) * t.z;

    return float3x3(
        focal.x / t.z, 0.0,           -(focal.x * t.x) / (t.z * t.z),
        0.0,           focal.y / t.z, -(focal.y * t.y) / (t.z * t.z),
        0.0,           0.0,          0.0, 
    );
}