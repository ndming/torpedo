import splat;
import utils;

[[vk::push_constant]]
uniform PointCloud pc; // push constant
uniform Camera camera; // set 0, binding 0

StructuredBuffer<GaussianPoint> gaussianPoints; // set 0, binding 1
RWStructuredBuffer<RasterPoint> rasterPoints;   // set 0, binding 2

// Based on: https://github.com/graphdeco-inria/gaussian-splatting
// Performs initial steps for each Gaussian prior to rasterization

[shader("compute")]
[numthreads(LOCAL_SCAN, 1, 1)]
void main(uint3 globalInvocationID : SV_DispatchThreadID) {
    // Each thread processes one Gaussian point
    let idx = globalInvocationID.x;
    if (idx >= pc.count) return;

    rasterPoints[idx].texel.w = 0.0; // reset radius
    rasterPoints[idx].tiles = 0;     // reset tile count

    // The Gaussian's center in world space
    let mean = gaussianPoints[idx].position;

    // Perform near culling, quit if outside the frustum
    float3 viewPos;
    if (!passNearCulling(mean, camera.viewMatrix, viewPos)) return;

    // Convert R and S to covariance 3D then project it to image space
    let focal = camera.imageSize / (2.0 * camera.tanFov);
    let cov3D = computeCovariance(gaussianPoints[idx].quaternion, gaussianPoints[idx].scale);
    let cov2D = projectCovariance(cov3D, mean, camera.viewMatrix, focal, camera.tanFov);

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

    // Bring Gaussian to NDC, note that projMatrix brings point from world all the way to NDC
    let clipPos = mul(camera.projMatrix, float4(mean, 1.0));
    let projPos = clipPos.xyz / (clipPos.w + EPSILON);

    // Use extent to compute a bounding rectangle of screen-space tiles that this Gaussian overlaps with
    let imgPoint = ndc2pix(projPos.xy, camera.imageSize);
    uint2 rectMin, rectMax;
    getBoundingRect(imgPoint, radius, getComputeGrid(camera.imageSize), rectMin, rectMax);
    if ((rectMax.x - rectMin.x) * (rectMax.y - rectMin.y) == 0) return; // empty tile

    // Compute color from spherical harmonics
    let direction = mean - camera.position;
    let color = evaluateSphericalHarmonics(gaussianPoints[idx].sh, direction, pc.shDegree);

    // Store preprocessed Gaussian data as RasterPoint
    rasterPoints[idx].color = color;
    rasterPoints[idx].texel = float4(imgPoint, viewPos.z, radius);
    rasterPoints[idx].copac = float4(conic, gaussianPoints[idx].opacity);
    rasterPoints[idx].tiles = (rectMax.x - rectMin.x) * (rectMax.y - rectMin.y);
}