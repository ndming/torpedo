module splatting;

public static const float EPSILON = 1e-7;
public static const uint  BLOCK_X = 16;
public static const uint  BLOCK_Y = 16;

__include "splat/common.slang";
__include "splat/surfel.slang";
__include "splat/volume.slang";

__include "utils/gaussian_covariance.slang";
__include "utils/spherical_harmonics.slang";

// Size: 240 bytes
// Alignment: 16 bytes
public struct GaussianPoint {
    public float3 position;
    public float opacity;
    public float4 quaternion;
    public float4 scale; // w = modifier
    public float sh[48]; // 3 * 16
}

// Size: 48 bytes
// Alignment: 16 bytes
public struct RasterPoint {
    public float3 color;
    public uint   tiles; // tiles touched
    public float4 texel; // texCoord + depth + radius
    public float4 copac; // conic + opacity
}

public struct PointCloud {
    public uint count;
    public uint shDegree;
}

public struct Camera {
    public float4x4 viewMatrix; // world to view space
    public float4x4 projMatrix; // world to clip space
    public float3 position;
    public float tanHalfFovX;
    public float tanHalfFovY;
    public uint imgW;
    public uint imgH;
}