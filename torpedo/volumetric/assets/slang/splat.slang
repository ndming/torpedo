module splat;

__include "splat/common.slang";
__include "splat/surfel.slang";
__include "splat/volume.slang";

public static const uint LINEAR_WORKGROUP_SIZE = 256;

public static const uint BLOCK_X = 16; // tile size in x
public static const uint BLOCK_Y = 16; // tile size in y

public struct PointCloud {
    public uint count;
    public uint shDegree; // active SH degree
    public uint2 imgSize; // in pixels
}

public struct Camera {
    public float4x4 viewMatrix; // world to view space, row-major
    public float4x4 projMatrix; // world to clip space, row-major
    public float2 tanHalfFov;   // tan (fov / 2) in x- and y-axis
}

// Size: 240 bytes, alignment: 16 bytes
public struct Gaussian {
    public float3 position;
    public float opacity;
    public float4 quaternion;
    public float4 scale; // w = modifier
    public float sh[48]; // 3 * 16
}

// Size: 48 bytes, alignment: 16 bytes
public struct Splat {
    public float3 color;
    public uint   tiles; // tiles touched
    public float4 texel; // texCoord + view depth + radius
    public float4 copac; // conic + opacity
}