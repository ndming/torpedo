#include "torpedo/foundation/StorageBuffer.h"

tpd::StorageBuffer tpd::StorageBuffer::Builder::build(const DeviceAllocator& allocator) const {
    auto allocation = VmaAllocation{};
    const auto buffer = creatBuffer(allocator, &allocation);
    return StorageBuffer{ buffer, allocation, allocator };
}

std::unique_ptr<tpd::StorageBuffer, tpd::foundation::Deleter<tpd::StorageBuffer>> tpd::StorageBuffer::Builder::build(
    const DeviceAllocator& allocator,
    std::pmr::memory_resource* pool) const
{
    auto allocation = VmaAllocation{};
    const auto buffer = creatBuffer(allocator, &allocation);
    return foundation::make_unique<StorageBuffer>(pool, buffer, allocation, allocator);
}

vk::Buffer tpd::StorageBuffer::Builder::creatBuffer(const DeviceAllocator& allocator, VmaAllocation* allocation) const {
    const auto usage = _usage | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst;
    const auto bufferCreateInfo = vk::BufferCreateInfo{ {}, _allocSize, usage };
    return allocator.allocateDeviceBuffer(bufferCreateInfo, allocation);
}
