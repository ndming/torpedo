// set 0, binding 0
WTexture2D outputImage;
// set 0, binding 1
StructuredBuffer<GaussianPoint> pointCloud;

struct GaussianPoint {
    float3 position;
    float opacity;
    float4 quaternion;
    float3 scale;
    float shDegree;
}

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
        color.xyz = pointCloud[1].quaternion.xyz;
    }
    
    outputImage.Store(texelCoord, color);
}