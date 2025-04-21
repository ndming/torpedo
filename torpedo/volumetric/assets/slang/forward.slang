import splat;

[[vk::binding(0)]]
WTexture2D outputImage;

[[vk::binding(3)]]
StructuredBuffer<Splat> splats;

[[vk::binding(9)]]
StructuredBuffer<uint32_t> indices;

[[vk::binding(10)]]
StructuredBuffer<uint2> ranges;

[shader("compute")]
[numthreads(BLOCK_X, BLOCK_Y, 1)]
void main(
    uint3 globalInvocationID : SV_DispatchThreadID,
    uint3 localInvocationID  : SV_GroupThreadID,
) {
    uint2 texelCoord = globalInvocationID.xy;
    uint2 imageSize; uint mipCount;
    outputImage.GetDimensions(0, imageSize.x, imageSize.y, mipCount);

    if (texelCoord.x < imageSize.x && texelCoord.y < imageSize.y) {
        float4 color = float4(0.0, 0.0, 0.0, 1.0);
        if (localInvocationID.x != 0 && localInvocationID.y != 0) {
            color.xyz = splats[0].color;
        }
        outputImage.Store(texelCoord, color);
    }
}