import splat;

WTexture2D outputImage;
StructuredBuffer<GaussianPoint> gaussianPoints;

[shader("compute")]
[numthreads(16, 16, 1)]
void main(
    uint3 globalInvocationID : SV_DispatchThreadID,
    uint3 localInvocationID  : SV_GroupThreadID,
) {
    uint2 texelCoord = globalInvocationID.xy;
    uint2 size; uint mipCount;
    outputImage.GetDimensions(0, size.x, size.y, mipCount);

    if(texelCoord.x < size.x && texelCoord.y < size.y) {
        float4 color = float4(0.0, 0.0, 0.0, 1.0);
        if (localInvocationID.x != 0 && localInvocationID.y != 0) {
            color.xyz = float3(gaussianPoints[1].sh[0], gaussianPoints[1].sh[1], gaussianPoints[1].sh[2]);
        }
        outputImage.Store(texelCoord, color);
    }
}