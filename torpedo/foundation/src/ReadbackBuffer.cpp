#include "torpedo/foundation/ReadbackBuffer.h"

tpd::ReadbackBuffer tpd::ReadbackBuffer::Builder::build(VmaAllocator allocator) const {
    auto allocation = VmaAllocation{};         // allocation is a pointer
    auto allocationInfo = VmaAllocationInfo{}; // allocation info is a struct

    const auto bufferCreateInfo = vk::BufferCreateInfo{ {}, _allocSize, _usage };
    const auto buffer = vma::allocateTwoWayBuffer(allocator, bufferCreateInfo, &allocation, &allocationInfo);

    return ReadbackBuffer{ allocationInfo.pMappedData, buffer, allocation };
}
