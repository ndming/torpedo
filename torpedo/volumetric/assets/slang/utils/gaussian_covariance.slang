implementing utils;

// Based on: https://github.com/graphdeco-inria/gaussian-splatting

/// Converts a `quaternion` and `scale` factors to a 3D covariance matrix in world space
public float3x3 computeCovariance(float4 quaternion, float3 scale, float scaleModifier) {
    // Create a scale matrix
    float3x3 S = float3x3(
        scale.x * scaleModifier, 0.0, 0.0,
        0.0, scale.y * scaleModifier, 0.0,
        0.0, 0.0, scale.z * scaleModifier,
    );

    // Unlike the original CUDA code, the w component is stored in the 4th position of float4 due to vsg's convention
    float w = quaternion.w, x = quaternion.x, y = quaternion.y, z = quaternion.z;
    // Compute rotation matrix from quaternion
    float3x3 R = float3x3(
        1.0 - 2.0 * (y * y + z * z), 2.0 * (x * y - w * z), 2.0 * (x * z + w * y),
        2.0 * (x * y + w * z), 1.0 - 2.0 * (x * x + z * z), 2.0 * (y * z - w * x),
        2.0 * (x * z - w * y), 2.0 * (y * z + w * x), 1.0 - 2.0 * (x * x + y * y),
    );

    // In the original CUDA code, the covariance matrix is computed as sigma = S * R
    // since they use glm which encodes matrices in column-major order
    float3x3 sigma = mul(R, S); // * in Slang is always element-wise multiplication 
    return mul(sigma, transpose(sigma)); // R * S * S^T * R^T
}

/// Projects a 3D covariance matrix of a Gaussian centered at `mean` in world space to a 2D covariance in clip space
public float2x2 projectCovariance(
    float3x3 cov3D,
    float3 mean,
    float4x4 viewMatrix,
    float focalX,
    float focalY,
    float tanHalfFovX,
    float tanHalfFovY,
) {
    float3x3 W = (float3x3)viewMatrix;
    float3x3 J = computeJacobian(mean, viewMatrix, focalX, focalY, tanHalfFovX, tanHalfFovY);
    float3x3 cov2D = mul(J, mul(W, mul(cov3D, transpose(W))) * transpose(J)); // J * W * Sigma_3D * W^T * J^T
    return (float2x2)cov2D;
}

/// Computes the local affine approximation of a projective transformation on `mean`, see EWA Spatting equation 29 
float3x3 computeJacobian(float3 mean, float4x4 viewMatrix, float focalX, float focalY, float tanHalfFovX, float tanHalfFovY) {
    // The Gaussian's center in camera space
    float3 t = mul(viewMatrix, float4(mean, 1.0)).xyz;

    // TODO: Understand this logic
    float limx = 1.3 * tanHalfFovX;
    float limy = 1.3 * tanHalfFovY;
    float txtz = t.x / t.z;
    float tytz = t.y / t.z;
    t.x = min(limx, max(-limx, txtz)) * t.z;
    t.y = min(limy, max(-limy, tytz)) * t.z;

    return float3x3(
        focalX / t.z, 0.0,          -(focalX * t.x) / (t.z * t.z),
        0.0,          focalY / t.z, -(focalY * t.y) / (t.z * t.z),
        0.0,          0.0,          0.0, 
    );
}