#include "torpedo/foundation/Buffer.h"

#include <numeric>
#include <ranges>

std::unique_ptr<tpd::Buffer> tpd::Buffer::Builder::build(const std::unique_ptr<ResourceAllocator>& allocator, const ResourceType type) {
    switch (type) {
        case ResourceType::Dedicated:  return buildDedicated(allocator);
        case ResourceType::Persistent: return buildPersistent(allocator);
        default: throw std::runtime_error("Buffer::Builder - Unsupported resource type");
    }
}

std::unique_ptr<tpd::Buffer> tpd::Buffer::Builder::buildDedicated(const std::unique_ptr<ResourceAllocator>& allocator) {
    auto allocation = VmaAllocation{};
    const auto buffer = allocator->allocateDedicatedBuffer(populateBufferCreateInfo(vk::BufferUsageFlagBits::eTransferDst), &allocation);
    return std::make_unique<Buffer>(buffer, allocation, std::move(_sizes));
}

std::unique_ptr<tpd::Buffer> tpd::Buffer::Builder::buildPersistent(const std::unique_ptr<ResourceAllocator> &allocator) {
    auto allocation = VmaAllocation{};          // allocation is a pointer
    auto allocationInfo = VmaAllocationInfo{};  // allocation info is a struct
    const auto buffer = allocator->allocatePersistentBuffer(populateBufferCreateInfo(), &allocation, &allocationInfo);
    return std::make_unique<Buffer>(buffer, allocation, std::move(_sizes), static_cast<std::byte*>(allocationInfo.pMappedData));
}

vk::BufferCreateInfo tpd::Buffer::Builder::populateBufferCreateInfo(const vk::BufferUsageFlags internalUsage) const {
    const auto bufferSize = std::reduce(_sizes.begin(), _sizes.end());

    auto bufferCreateInfo = vk::BufferCreateInfo{};
    bufferCreateInfo.size = bufferSize;
    bufferCreateInfo.usage = _usage | internalUsage;

    return bufferCreateInfo;
}

void tpd::Buffer::transferBufferData(
    const uint32_t bufferIndex,
    const void* const data,
    const std::unique_ptr<ResourceAllocator>& allocator,
    const std::function<void(vk::Buffer, vk::Buffer, vk::BufferCopy)>& onBufferCopy) const
{
    if (_pMappedData) {
        throw std::runtime_error("Buffer - Buffer was persistently created, transfer only works for dedicated buffers");
    }

    // Create a staging buffer and fill it with the data
    auto stagingAllocation = VmaAllocation{};
    const auto stagingBuffer = allocator->allocateStagingBuffer(_sizes[bufferIndex], &stagingAllocation);
    allocator->mapAndCopyStagingBuffer(_sizes[bufferIndex], data, stagingAllocation);

    // Copy the content from the staging buffer to the dedicated buffer
    const auto offset = std::ranges::fold_left(_sizes | std::views::take(bufferIndex), 0, std::plus());
    onBufferCopy(stagingBuffer, _buffer, vk::BufferCopy{ 0, offset, _sizes[bufferIndex] });

    allocator->freeBuffer(stagingBuffer, stagingAllocation);
}

void tpd::Buffer::updateBufferData(const uint32_t bufferIndex, const void* const data, const std::size_t dataByteSize) const {
    if (!_pMappedData) {
        throw std::runtime_error("Buffer - Buffer was locally created, update only works for persistent buffers");
    }

    // Memory in Vulkan doesn't need to be unmapped before using it on GPU. However, unless memory types have
    // a VK_MEMORY_PROPERTY_HOST_COHERENT_BIT flag set, we need to manually invalidate cache before reading of mapped
    // pointer and flush cache after writing to map a pointer. Map/unmap operations don't do that automatically.
    // Windows drivers from all 3 PC GPU vendors (AMD, Intel, NVIDIA) currently provide HOST_COHERENT flag on all
    // memory types that are HOST_VISIBLE, so on PC we may not need to bother.
    const auto offset = std::ranges::fold_left(_sizes | std::views::take(bufferIndex), 0, std::plus());
    memcpy(_pMappedData + offset, data, dataByteSize);
}
