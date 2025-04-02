[[vk::binding(0, 0)]]
WTexture2D outputImage;

struct GaussianPoint {
    float3 position;
    float4 quaternion;
    float3 scale;
    float opacity;
}

[[vk::binding(1, 0)]]
StructuredBuffer<GaussianPoint> pointCloud;

[shader("compute")]
[numthreads(16, 16, 1)]
void main(
    uint3 globalInvocationID : SV_DispatchThreadID,
    uint3 localInvocationID  : SV_GroupThreadID,
) {
    uint2 texelCoord = globalInvocationID.xy;
    uint2 size; uint mipCount;
    outputImage.GetDimensions(0, size.x, size.y, mipCount);

    float4 color = float4(0.0, 0.0, 0.0, 1.0);
    if (localInvocationID.x != 0 && localInvocationID.y != 0) {
        color.x = float(texelCoord.x) / (size.x);
        color.y = float(texelCoord.y) / (size.y);	
    }
    
    outputImage.Store(texelCoord, color);
}