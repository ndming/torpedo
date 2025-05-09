import splat;

[[vk::push_constant]]
uniform RasterInfo info;

[[vk::binding(0)]]
WTexture2D outputImage;

[[vk::binding(1)]]
ConstantBuffer<Camera> camera;

[[vk::binding(2)]]
StructuredBuffer<Gaussian> gaussians;

[[vk::binding(3)]]
RWStructuredBuffer<Splat> splats;

// Projects Gaussian points to Splat points on image space
// Based on: https://github.com/graphdeco-inria/gaussian-splatting

[shader("compute")]
[numthreads(WORKGROUP_SIZE, 1, 1)]
void main(uint3 globalInvocationID : SV_DispatchThreadID) {
    // Each thread processes one Gaussian point
    let idx = globalInvocationID.x;
    if (idx >= info.pointCount) return;

    splats[idx].texel.w = 0.0; // reset radius
    splats[idx].tiles = 0;     // reset tile count

    // The Gaussian's center in world space
    let mean = gaussians[idx].position;

    // Quit if outside the frustum
    float depth; float3 projPos; // depth z in view space and projected position in NDC
    if (!passFrustumClipping(mean, camera.viewMatrix, camera.projMatrix, depth, projPos)) return;

    // The image size in pixels
    uint2 imageSize; uint mipCount;
    outputImage.GetDimensions(0, imageSize.x, imageSize.y, mipCount);

    // Convert R and S to covariance 3D then project it to image space
    let focal = imageSize / (2.0 * camera.tanHalfFov);
    let cov3D = computeCovariance(gaussians[idx].quaternion, gaussians[idx].scale);
    let cov2D = projectCovariance(cov3D, mean, camera.viewMatrix, focal, camera.tanHalfFov);

    // Apply low-pass filter: every Gaussian should be at least one pixel wide/high, discard 3rd row and column
    let cov = float3(cov2D[0][0] + 0.3, cov2D[1][0], cov2D[1][1] + 0.3f);

    // Invert covariance (EWA algorithm)
    let det = cov.x * cov.z - cov.y * cov.y;
    if (det == 0.0) return; // singular matrix
    let conic = float3(cov.z, -cov.y, cov.x) / det;

    // Compute extent in screen space (by finding eigenvalues of cov2D)
    let mid = 0.5 * (cov.x + cov.z);
    let lambda_1 = mid + sqrt(max(0.1, mid * mid - det));
    let lambda_2 = mid - sqrt(max(0.1, mid * mid - det));
    let radius = ceil(3.0 * sqrt(max(lambda_1, lambda_2))); // in pixels

    // Use extent to compute a bounding rectangle of screen-space tiles that this Gaussian overlaps with
    let imgPoint = ndc2pix(projPos.xy, imageSize);
    uint2 rectMin, rectMax;
    getBoundingRect(imgPoint, radius, getComputeGrid(imageSize), rectMin, rectMax);
    if ((rectMax.x - rectMin.x) * (rectMax.y - rectMin.y) == 0) return; // empty tile

    // Compute color from spherical harmonics
    let direction = mean - getCameraWorldPosition(camera.viewMatrix);
    let color = evaluateSphericalHarmonics(gaussians[idx].sh, direction, info.shDegree);

    // Store preprocessed Gaussian data as RasterPoint
    splats[idx].color = color;
    splats[idx].texel = float4(imgPoint, depth, radius);
    splats[idx].copac = float4(conic, gaussians[idx].opacity);
    splats[idx].tiles = (rectMax.x - rectMin.x) * (rectMax.y - rectMin.y);
}