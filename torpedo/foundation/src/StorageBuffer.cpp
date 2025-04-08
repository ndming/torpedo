#include "torpedo/foundation/StorageBuffer.h"

tpd::StorageBuffer tpd::StorageBuffer::Builder::build(const DeviceAllocator& allocator) const {
    auto allocation = VmaAllocation{};
    const auto buffer = creatBuffer(allocator, &allocation);
    // If the call site set data but not its size, we default to the buffer's size
    const auto dataSize = _data && _dataSize == 0 ? static_cast<uint32_t>(_allocSize) : _dataSize;
    return StorageBuffer{ _dstPoint, _allocSize, buffer, allocation, allocator, _data, dataSize };
}

std::unique_ptr<tpd::StorageBuffer, tpd::Deleter<tpd::StorageBuffer>> tpd::StorageBuffer::Builder::build(
    const DeviceAllocator& allocator,
    std::pmr::memory_resource* pool) const
{
    auto allocation = VmaAllocation{};
    const auto buffer = creatBuffer(allocator, &allocation);
    const auto dataSize = _data && _dataSize == 0 ? static_cast<uint32_t>(_allocSize) : _dataSize;
    return foundation::make_unique<StorageBuffer>(pool, _dstPoint, _allocSize, buffer, allocation, allocator, _data, dataSize);
}

vk::Buffer tpd::StorageBuffer::Builder::creatBuffer(const DeviceAllocator& allocator, VmaAllocation* allocation) const {
    const auto usage = _usage | vk::BufferUsageFlagBits::eStorageBuffer;
    const auto bufferCreateInfo = vk::BufferCreateInfo{ {}, _allocSize, usage, vk::SharingMode::eExclusive };
    return allocator.allocateDeviceBuffer(bufferCreateInfo, allocation);
}

void tpd::StorageBuffer::recordBufferTransfer(const vk::CommandBuffer cmd, const vk::Buffer stagingBuffer) const noexcept {
    const auto bufferCopyInfo = vk::BufferCopy{}
        .setSrcOffset(0)
        .setDstOffset(0)
        .setSize(getSyncDataSize());
    cmd.copyBuffer(stagingBuffer, _buffer, bufferCopyInfo);
}

void tpd::StorageBuffer::recordTransferDstSync(const vk::CommandBuffer cmd) const noexcept {
    auto barrier = vk::BufferMemoryBarrier2{};
    barrier.buffer = _buffer;
    barrier.offset = 0;
    barrier.size   = getSyncDataSize();
    barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.srcStageMask  = vk::PipelineStageFlagBits2::eTransfer;
    barrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
    barrier.dstStageMask  = _transferDstPoint.stage;
    barrier.dstAccessMask = _transferDstPoint.access;

    const auto dependency = vk::DependencyInfo{}.setBufferMemoryBarriers(barrier);
    cmd.pipelineBarrier2(dependency);
}

void tpd::StorageBuffer::recordOwnershipRelease(
    const vk::CommandBuffer cmd,
    const uint32_t srcFamilyIndex,
    const uint32_t dstFamilyIndex) const noexcept
{
    auto barrier = vk::BufferMemoryBarrier2{};
    barrier.buffer = _buffer;
    barrier.offset = 0;
    barrier.size   = vk::WholeSize;
    barrier.srcQueueFamilyIndex = srcFamilyIndex;
    barrier.dstQueueFamilyIndex = dstFamilyIndex;
    barrier.srcStageMask  = vk::PipelineStageFlagBits2::eTransfer;  // releasing after buffer uploading
    barrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;    // ensure memory write is available

    // TODO: consider VK_DEPENDENCY_QUEUE_FAMILY_OWNERSHIP_TRANSFER_USE_ALL_STAGES_BIT_KHR to avoid full pipeline stalls
    const auto dependency = vk::DependencyInfo{}.setBufferMemoryBarriers(barrier);
    cmd.pipelineBarrier2(dependency);
}

void tpd::StorageBuffer::recordOwnershipAcquire(
    const vk::CommandBuffer cmd,
    const uint32_t srcFamilyIndex,
    const uint32_t dstFamilyIndex) const noexcept
{
    // Make the buffer accessible from all shaders
    auto barrier = vk::BufferMemoryBarrier2{};
    barrier.buffer = _buffer;
    barrier.offset = 0;
    barrier.size   = vk::WholeSize;
    barrier.srcQueueFamilyIndex = srcFamilyIndex;
    barrier.dstQueueFamilyIndex = dstFamilyIndex;
    barrier.dstStageMask  = _transferDstPoint.stage;
    barrier.dstAccessMask = _transferDstPoint.access;

    // TODO: consider VK_DEPENDENCY_QUEUE_FAMILY_OWNERSHIP_TRANSFER_USE_ALL_STAGES_BIT_KHR to avoid full pipeline stalls
    const auto dependency = vk::DependencyInfo{}.setBufferMemoryBarriers(barrier);
    cmd.pipelineBarrier2(dependency);
}
