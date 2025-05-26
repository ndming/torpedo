module splat;

__include "splat/common.slang";
__include "splat/surfel.slang";
__include "splat/volume.slang";

public static const uint WORKGROUP_SIZE = 256; // workgroup size for all passes before blending
public static const uint BLOCK_X = 16; // tile size in x-dimension in blending pass
public static const uint BLOCK_Y = 16; // tile size in x-dimension in blending pass

public struct RasterInfo {
    public uint pointCount; // number of Gaussian points
    public uint shDegree;   // active SH degree
    public uint tilesRendered; // `numRendered` in the CUDA code
    public uint sortIteration; // current sort iteration
}

public struct Camera {
    public float4x4 viewMatrix; // world to view space, row-major
    public float4x4 projMatrix; // world to clip space, row-major
    public float2 focalNDC; // inverse tangent of half FOV
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
    public float4 texel; // image point + view depth + radius
    public float4 copac; // conic + opacity
}