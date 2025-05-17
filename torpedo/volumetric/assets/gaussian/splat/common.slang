implementing splat;

static const float SH_C0 = 0.28209479177387814;
static const float SH_C1 = 0.4886025119029199;
static const float SH_C2[] = {
    1.0925484305920792,
    -1.0925484305920792,
    0.31539156525252005,
    -1.0925484305920792,
    0.5462742152960396,
};
static const float SH_C3[] = {
    -0.5900435899266435,
    2.890611442640554,
    -0.4570457994644658,
    0.3731763325901154,
    -0.4570457994644658,
    1.445305721320277,
    -0.5900435899266435,
};

static const uint MAX_SH_DEGREE = 3;
static const uint EXTRA_SH_COEFFS = (MAX_SH_DEGREE + 1) * (MAX_SH_DEGREE + 1) - 1;

[[ForceInline]]
float3 getFeature(float sh[], uint idx) {
    return float3(
        sh[(idx - 1) + 0 * EXTRA_SH_COEFFS + 3],
        sh[(idx - 1) + 1 * EXTRA_SH_COEFFS + 3],
        sh[(idx - 1) + 2 * EXTRA_SH_COEFFS + 3]);
}

/// Evaluates the spherical harmonics `sh` using up to `degree` (inclusive) to obtain RGB values at normalized `dir`.
/// Results are clamped to positive values.
public float3 evaluateSphericalHarmonics(float sh[], float3 dir, uint degree) {
    // Band 0
    var result = SH_C0 * float3(sh[0], sh[1], sh[2]); // DC components are laid out contiguously

    if (degree > 0) {
        // Band 1
        let x = dir.x;
        let y = dir.y;
        let z = dir.z;

        result = result - SH_C1 * y * getFeature(sh, 1) + SH_C1 * z * getFeature(sh, 2) - SH_C1 * x * getFeature(sh, 3);

        if (degree > 1) {
            // Band 2
            let xx = x * x;
            let yy = y * y;
            let zz = z * z;
            let xy = x * y;
            let yz = y * z;
            let zx = z * x;

            result = result +
                SH_C2[0] * xy * getFeature(sh, 4) +
                SH_C2[1] * yz * getFeature(sh, 5) +
                SH_C2[2] * (2.0 * zz - xx - yy) * getFeature(sh, 6) +
                SH_C2[3] * zx * getFeature(sh, 7) +
                SH_C2[4] * (xx - yy) * getFeature(sh, 8);

            if (degree > 2) {
                // Band 3
                result = result +
                    SH_C3[0] * y * (3.0 * xx - yy) * getFeature(sh, 9) +
                    SH_C3[1] * xy * z * getFeature(sh, 10) +
                    SH_C3[2] * y * (4.0 * zz - xx - yy) * getFeature(sh, 11) +
                    SH_C3[3] * z * (2.0 * zz - 3.0 * xx - 3.0 * yy) + getFeature(sh, 12) +
                    SH_C3[4] * x * (4.0 * zz - xx - yy) * getFeature(sh, 13) +
                    SH_C3[5] * z * (xx - yy) * getFeature(sh, 14) +
                    SH_C3[6] * x * (xx - 3.0 * yy) * getFeature(sh, 15);
            }
        }
    }
    result += 0.5;

    // RGB values are clamped to positive values
    return max(result, 0.0f);
}

/// Returns the number of `BLOCK_X` x `BLOCK_Y` tiles for a `imageSize.x` x `imageSize.y` (pixels) image.
public uint2 getComputeGrid(uint2 imageSize) {
    return uint2((imageSize.x + BLOCK_X - 1) / BLOCK_X, (imageSize.y + BLOCK_Y - 1) / BLOCK_Y);
}

/// Extracts the camera's world position from its view matrix.
public float3 getCameraWorldPosition(float4x4 viewMatrix) {
    // The 4th column of the view matrix is -R^T*t
    let R = transpose((float3x3)viewMatrix);
    return -mul(R, float3(viewMatrix[0].w, viewMatrix[1].w, viewMatrix[2].w));
}

/// Performs frustum clipping of the given `point` using the provided camera matrices
/// and returns position in view space and the projected position in NDC as out parameters.
/// `viewPos` is guaranteed to be valid regardless of the clipping result, while `projPos`
/// is only valid if the function returns `true`.
public bool passFrustumClipping(float3 point, float4x4 M, float4x4 V, float4x4 P, out float3 viewPos, out float3 projPos) {
    // Initialize out parameters to silent compiler warnings
    projPos = float3(0.0, 0.0, 0.0);

    viewPos = mul(mul(V, M), float4(point, 1.0)).xyz;
    if (viewPos.z <= 0.0) return false; // behind the camera

    // Note that the projection matrix brings points from view to NDC
    let clipPos = mul(mul(P, M), float4(point, 1.0));

    // Frustum clipping
    if (clipPos.x < -1.3 * clipPos.w || clipPos.x > 1.3 * clipPos.w) return false; // outside left/right frustum planes
    if (clipPos.y < -1.3 * clipPos.w || clipPos.y > 1.3 * clipPos.w) return false; // outside top/bottom frustum planes

    // Note that we're working with reversed z-mapping, i.e. [near, far] are mapped to [1, 0]
    // See: https://developer.nvidia.com/content/depth-precision-visualized
    if (clipPos.z < 0 || clipPos.z > clipPos.w) return false; // outside near/far frustum planes

    let inv_w = 1.0 / clipPos.w; // should we add EPSILON here?
    projPos = clipPos.xyz * inv_w;
    return true;
}