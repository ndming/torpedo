import splat;

[[vk::binding(10)]]
RWStructuredBuffer<uint2> ranges;

[shader("compute")]
[numthreads(WORKGROUP_SIZE, 1, 1)]
void main(uint3 globalInvocationID : SV_DispatchThreadID) {
    let idx = globalInvocationID.x;
    if (idx >= ranges.getCount()) return;
    ranges[idx] = uint2(0, 0);
}