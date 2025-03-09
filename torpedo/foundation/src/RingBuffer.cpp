#include "torpedo/foundation/RingBuffer.h"

tpd::RingBuffer tpd::RingBuffer::Builder::build(const DeviceAllocator& allocator) const {
    auto allocation = VmaAllocation{};          // allocation is a pointer
    auto allocationInfo = VmaAllocationInfo{};  // allocation info is a struct

    const auto bufferCreateInfo = vk::BufferCreateInfo{ {}, _allocSize * _bufferCount, _usage };
    const auto buffer = allocator.allocateMappedBuffer(bufferCreateInfo, &allocation, &allocationInfo);

    return RingBuffer{
        static_cast<std::byte*>(allocationInfo.pMappedData), _bufferCount,
        _bufferSize, _allocSize, buffer, allocation, allocator };
}

std::unique_ptr<tpd::RingBuffer, tpd::foundation::Deleter<tpd::RingBuffer>> tpd::RingBuffer::Builder::build(
    const DeviceAllocator& allocator,
    std::pmr::memory_resource* pool) const
{
    auto allocation = VmaAllocation{};          // allocation is a pointer
    auto allocationInfo = VmaAllocationInfo{};  // allocation info is a struct

    const auto bufferCreateInfo = vk::BufferCreateInfo{ {}, _allocSize * _bufferCount, _usage };
    const auto buffer = allocator.allocateMappedBuffer(bufferCreateInfo, &allocation, &allocationInfo);

    return foundation::make_unique<RingBuffer>(
        pool, static_cast<std::byte*>(allocationInfo.pMappedData), _bufferCount,
        _bufferSize, _allocSize, buffer, allocation, allocator);
}

void tpd::RingBuffer::updateData(const uint32_t bufferIndex, const void* data, const std::size_t byteSize) const {
    // Memory in Vulkan doesn't need to be unmapped before using it on GPU. However, unless memory types have
    // a VK_MEMORY_PROPERTY_HOST_COHERENT_BIT flag set, we need to manually invalidate cache before reading of mapped
    // pointer and flush cache after writing to map a pointer. Map/unmap operations don't do that automatically.
    // Windows drivers from all 3 PC GPU vendors (AMD, Intel, NVIDIA) currently provide HOST_COHERENT flag on all
    // memory types that are HOST_VISIBLE, so on PC we may not need to bother.
    const auto size = byteSize == 0 ? _perBufferSize : byteSize;
    memcpy(_pMappedData + bufferIndex * _perAllocSize, data, size);
}

void tpd::RingBuffer::updateData(const void* data, const std::size_t byteSize) const {
    for (auto i = 0; i < _bufferCount; ++i) {
        updateData(i, data, byteSize);
    }
}
