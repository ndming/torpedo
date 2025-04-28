implementing splat;

/// Avoid zero division in perspective projection
public static const float EPSILON = 1e-7;

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

[[ForceInline]]
float3 getSHCoeff(float sh[], uint idx) {
    return float3(sh[idx * 3 + 0], sh[idx * 3 + 1], sh[idx * 3 + 2]);
}

/// Evaluates the spherical harmonics `sh` using up to `degree` (inclusive) to obtain RGB values at `direction`.
/// Results are clamped to positive values.
public float3 evaluateSphericalHarmonics(float sh[], float3 direction, uint degree) {
    let dir = normalize(direction);

    // Band 0
    float3 result = SH_C0 * getSHCoeff(sh, 0);

    if (degree > 0) {
        // Band 1
        float x = dir.x, y = dir.y, z = dir.z;
        result = result - SH_C1 * y * getSHCoeff(sh, 1) + SH_C1 * z * getSHCoeff(sh, 2) - SH_C1 * x * getSHCoeff(sh, 3);

        if (degree > 1) {
            // Band 2
            float xx = x * x, yy = y * y, zz = z * z;
            float xy = x * y, yz = y * z, zx = z * x;
            result = result + 
                SH_C2[0] * xy * getSHCoeff(sh, 4) +
                SH_C2[1] * yz * getSHCoeff(sh, 5) +
                SH_C2[2] * (2.0 * zz - xx - yy) * getSHCoeff(sh, 6) +
                SH_C2[3] * zx * getSHCoeff(sh, 7) +
                SH_C2[4] * (xx - yy) * getSHCoeff(sh, 8);

            if (degree > 2) {
                // Band 3
                result = result +
                    SH_C3[0] * y * (3.0 * xx - yy) * getSHCoeff(sh, 9) +
                    SH_C3[1] * xy * z * getSHCoeff(sh, 10) +
                    SH_C3[2] * y * (4.0 * zz - xx - yy) * getSHCoeff(sh, 11) +
                    SH_C3[3] * z * (2.0 * zz - 3.0 * xx - 3.0 * yy) + getSHCoeff(sh, 12) + 
                    SH_C3[4] * x * (4.0 * zz - xx - yy) * getSHCoeff(sh, 13) +
                    SH_C3[5] * z * (xx - yy) * getSHCoeff(sh, 14) +
                    SH_C3[6] * x * (xx - 3.0 * yy) * getSHCoeff(sh, 15);
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
    return -mul(R, viewMatrix[3].xyz);
}