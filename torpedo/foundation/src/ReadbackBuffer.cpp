#include "torpedo/foundation/ReadbackBuffer.h"

tpd::ReadbackBuffer tpd::ReadbackBuffer::Builder::build(const DeviceAllocator& allocator) const {
    auto allocation = VmaAllocation{};         // allocation is a pointer
    auto allocationInfo = VmaAllocationInfo{}; // allocation info is a struct

    const auto bufferCreateInfo = vk::BufferCreateInfo{ {}, _allocSize, _usage };
    const auto buffer = allocator.allocateTwoWayBuffer(bufferCreateInfo, &allocation, &allocationInfo);

    return ReadbackBuffer{ allocationInfo.pMappedData, _allocSize, buffer, allocation, allocator };
}

std::unique_ptr<tpd::ReadbackBuffer, tpd::Deleter<tpd::ReadbackBuffer>> tpd::ReadbackBuffer::Builder::build(
    const DeviceAllocator& allocator,
    std::pmr::memory_resource* pool) const
{
    auto allocation = VmaAllocation{};         // allocation is a pointer
    auto allocationInfo = VmaAllocationInfo{}; // allocation info is a struct

    const auto bufferCreateInfo = vk::BufferCreateInfo{ {}, _allocSize, _usage };
    const auto buffer = allocator.allocateTwoWayBuffer(bufferCreateInfo, &allocation, &allocationInfo);

    return foundation::make_unique<ReadbackBuffer>(pool, allocationInfo.pMappedData, _allocSize, buffer, allocation, allocator);
}