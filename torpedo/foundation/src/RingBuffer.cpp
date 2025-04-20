#include "torpedo/foundation/RingBuffer.h"

tpd::RingBuffer tpd::RingBuffer::Builder::build(VmaAllocator allocator) const {
    auto allocation = VmaAllocation{};         // allocation is a pointer
    auto allocationInfo = VmaAllocationInfo{}; // allocation info is a struct

    const auto bufferCreateInfo = vk::BufferCreateInfo{ {}, _allocSize * _bufferCount, _usage };
    const auto buffer = vma::allocateMappedBuffer(allocator, bufferCreateInfo, &allocation, &allocationInfo);

    const auto allocSizePerBuffer = static_cast<uint32_t>(_allocSize);
    const auto pMappedData = static_cast<std::byte*>(allocationInfo.pMappedData);
    return RingBuffer{ pMappedData, _bufferCount, allocSizePerBuffer, buffer, allocation };
}

void tpd::RingBuffer::update(const uint32_t bufferIndex, const void* data, const std::size_t size) const {
    // Memory in Vulkan doesn't need to be unmapped before using it on GPU. However, unless memory types have
    // a VK_MEMORY_PROPERTY_HOST_COHERENT_BIT flag set, we need to manually invalidate cache before reading of mapped
    // pointer and flush cache after writing to map a pointer. Map/unmap operations don't do that automatically.
    // Windows drivers from all 3 PC GPU vendors (AMD, Intel, NVIDIA) currently provide HOST_COHERENT flag on all
    // memory types that are HOST_VISIBLE, so on PC we may not need to bother.
    memcpy(_pMappedData + _allocSizePerBuffer * bufferIndex, data, size);
}

void tpd::RingBuffer::update(const void* data, const std::size_t size) const {
    for (auto i = 0; i < _bufferCount; ++i) {
        update(i, data, size);
    }
}

uint32_t tpd::RingBuffer::getOffset(const uint32_t bufferIndex) const {
    if (bufferIndex >= _bufferCount) [[unlikely]] {
        throw std::out_of_range("RingBuffer - Could NOT get offset to a buffer whose index is out of range");
    }
    return _allocSizePerBuffer * bufferIndex;
}
