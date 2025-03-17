#include "torpedo/foundation/StagingBuffer.h"

tpd::StagingBuffer tpd::StagingBuffer::Builder::build(const DeviceAllocator& allocator) const {
    auto allocation = VmaAllocation{};
    const auto buffer = allocator.allocateStagedBuffer(_allocSize, &allocation);
    return StagingBuffer{ _bufferSize, buffer, allocation, allocator };
}

std::unique_ptr<tpd::StagingBuffer, tpd::Deleter<tpd::StagingBuffer>> tpd::StagingBuffer::Builder::build(
    const DeviceAllocator& allocator,
    std::pmr::memory_resource* pool) const
{
    auto allocation = VmaAllocation{};
    const auto buffer = allocator.allocateStagedBuffer(_allocSize, &allocation);
    return foundation::make_unique<StagingBuffer>(pool, _bufferSize, buffer, allocation, allocator);
}

void tpd::StagingBuffer::setData(const void* const data, const std::size_t byteSize) const {
    void* mappedData = _allocator.mapMemory(_allocation);
    const auto size = byteSize == 0 ? _bufferSize : byteSize;
    // Memory write by host is guaranteed to be visible
    // prior to the next queue submit
    memcpy(mappedData, data, size);
    _allocator.unmapMemory(_allocation);
}
