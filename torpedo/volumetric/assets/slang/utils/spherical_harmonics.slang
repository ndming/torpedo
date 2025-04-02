implementing utils;

// Based on: https://github.com/graphdeco-inria/gaussian-splatting

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

public struct SphericalHarmonics {
    // Band 0
    float3 coeff0;
    // Band 1
    float3 coeff1;
    float3 coeff2;
    float3 coeff3;
    // Band2
    float3 coeff4;
    float3 coeff5;
    float3 coeff6;
    float3 coeff7;
    float3 coeff8;
    // Band3
    float3 coeff9;
    float3 coeff10;
    float3 coeff11;
    float3 coeff12;
    float3 coeff13;
    float3 coeff14;
    float3 coeff15;
};

/// Evaluates the spherical harmonics `sh` at the given `direction`, up to the given `degree` (inclusive)
public float3 evaluateSphericalHarmonics(SphericalHarmonics sh, float3 direction, uint degree, bool clampToPositive = true) {
    float3 dir = normalize(direction);

    float3 result = SH_C0 * sh.coeff0;

    if (degree > 0) {
        float x = dir.x, y = dir.y, z = dir.z;
        result = result - SH_C1 * y * sh.coeff1 + SH_C1 * z * sh.coeff2 - SH_C1 * x * sh.coeff3;

        if (degree > 1) {
            float xx = x * x, yy = y * y, zz = z * z;
            float xy = x * y, yz = y * z, zx = z * x;
            result = result + 
                SH_C2[0] * xy * sh.coeff4 +
                SH_C2[1] * yz * sh.coeff5 +
                SH_C2[2] * (2.0 * zz - xx - yy) * sh.coeff6 +
                SH_C2[3] * zx * sh.coeff7 +
                SH_C2[4] * (xx - yy) * sh.coeff8;

            if (degree > 2) {
                result = result +
                    SH_C3[0] * y * (3.0 * xx - yy) * sh.coeff9 +
                    SH_C3[1] * xy * z * sh.coeff10 +
                    SH_C3[2] * y * (4.0 * zz - xx - yy) * sh.coeff11 +
                    SH_C3[3] * z * (2.0 * zz - 3.0 * xx - 3.0 * yy) + sh.coeff12 + 
                    SH_C3[4] * x * (4.0 * zz - xx - yy) * sh.coeff13 +
                    SH_C3[5] * z * (xx - yy) * sh.coeff14 +
                    SH_C3[6] * x * (xx - 3.0 * yy) * sh.coeff15;
            }
        }
    }
    result += 0.5;

    if (clampToPositive) {
        result = max(result, 0.0f);
    }
    return result;
}