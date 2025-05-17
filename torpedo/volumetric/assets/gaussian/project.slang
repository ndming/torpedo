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

[[vk::binding(0, 1)]] // set 1, binding 0
uniform StructuredBuffer<ConstantBuffer<float4[4]>.Handle> transforms; // this is going to be a handle array of uint2

[[vk::binding(1, 1)]] // set 1, binding 1
StructuredBuffer<uint> transformIndices;

// There's going to be a global uniform buffer array at set 2, binding 0,
// generated automatically by the compiler due to our handle declaration.
// layout(std140) uniform Transform {
// vec4[4] data;
// } transforms[];

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

    // Acquire the model matrix using descriptor indexing, see:
    // https://shader-slang.org/slang/user-guide/convenience-features.html#descriptorhandle-for-bindless-descriptor-access
    // https://docs.vulkan.org/samples/latest/samples/extensions/descriptor_indexing/README.html
    let r = *nonuniform(transforms[transformIndices[idx]]);
    let model = float4x4(r[0], r[1], r[2], r[3]);

    // Quit if outside the frustum
    float3 viewPos; float3 projPos; // depth z in view space and projected position in NDC
    if (!passFrustumClipping(mean, model, camera.viewMatrix, camera.projMatrix, viewPos, projPos)) return;

    // The image size in pixels
    uint2 imageSize; uint mipCount;
    outputImage.GetDimensions(0, imageSize.x, imageSize.y, mipCount);

    // Convert R and S to covariance 3D then project it to image space
    let focal = 0.5 * imageSize * camera.focalNDC;
    let cov3D = computeCovariance(gaussians[idx].quaternion, gaussians[idx].scale);
    let cov2D = projectCovariance(cov3D, viewPos, camera.viewMatrix, focal);

    // Apply low-pass filter: every Gaussian should be at least one pixel wide/high, discard 3rd row and column
    let cov = float3(cov2D[0][0] + 0.3f, cov2D[1][0], cov2D[1][1] + 0.3f);

    // Invert covariance (EWA algorithm)
    let det = cov.x * cov.z - cov.y * cov.y;
    if (det == 0.0) return; // singular matrix
    let det_inv = 1.0 / det;
    let conic = float3(cov.z, -cov.y, cov.x) * det_inv;

    // Compute extent in screen space (by finding eigenvalues of cov2D)
    let mid = 0.5 * (cov.x + cov.z);
    let lambda_1 = mid + sqrt(max(0.1, mid * mid - det));
    let lambda_2 = mid - sqrt(max(0.1, mid * mid - det));
    let radius = ceil(3.0 * sqrt(max(lambda_1, lambda_2))); // in pixels

    // Use extent to compute a bounding rectangle of screen-space tiles that this Gaussian overlaps with
    let imgPoint = ndc2pix(projPos.xy, imageSize);
    uint2 rectMin, rectMax;
    getBoundingRect(imgPoint, radius, getComputeGrid(imageSize), rectMin, rectMax);
    let touchedTiles = (rectMax.x - rectMin.x) * (rectMax.y - rectMin.y);
    if (touchedTiles == 0) return; // empty tile

    // Compute color from spherical harmonics
    let direction = normalize(mean - getCameraWorldPosition(camera.viewMatrix));
    let color = evaluateSphericalHarmonics(gaussians[idx].sh, direction, info.shDegree);

    // Store preprocessed Gaussian data as RasterPoint
    splats[idx].color = color;
    splats[idx].texel = float4(imgPoint, viewPos.z, radius);
    splats[idx].copac = float4(conic, gaussians[idx].opacity);
    splats[idx].tiles = touchedTiles;
}