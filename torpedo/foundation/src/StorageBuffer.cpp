#include "torpedo/foundation/StorageBuffer.h"

#include <ranges>

tpd::StorageBuffer tpd::StorageBuffer::Builder::build(VmaAllocator allocator) const {
    const auto usage = _usage | vk::BufferUsageFlagBits::eStorageBuffer;
    const auto bufferCreateInfo = vk::BufferCreateInfo{ {}, _allocSize, usage, vk::SharingMode::eExclusive };

    auto allocation = VmaAllocation{};
    const auto buffer = vma::allocateDeviceBuffer(allocator, bufferCreateInfo, &allocation);

    return StorageBuffer{ buffer, allocation };
}

void tpd::StorageBuffer::recordStagingCopy(
    const vk::CommandBuffer cmd,
    const vk::Buffer stagingBuffer,
    const vk::DeviceSize size) const noexcept 
{
    auto bufferCopyInfo = vk::BufferCopy{};
    bufferCopyInfo.srcOffset = 0;
    bufferCopyInfo.dstOffset = 0;
    bufferCopyInfo.size = size;
    cmd.copyBuffer(stagingBuffer, _resource, bufferCopyInfo);
}

void tpd::StorageBuffer::recordTransferDstPoint(const vk::CommandBuffer cmd, const SyncPoint dstSync) const noexcept {
    auto barrier = vk::BufferMemoryBarrier2{};
    barrier.buffer = _resource;
    barrier.offset = 0;
    barrier.size = vk::WholeSize;
    barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.srcStageMask  = vk::PipelineStageFlagBits2::eTransfer;
    barrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
    barrier.dstStageMask  = dstSync.stage;
    barrier.dstAccessMask = dstSync.access;

    const auto dependency = vk::DependencyInfo{}.setBufferMemoryBarriers(barrier);
    cmd.pipelineBarrier2(dependency);
}

void tpd::StorageBuffer::recordComputeDstAccess(
    const vk::CommandBuffer cmd,
    const vk::AccessFlags2 dstAccess) const noexcept
{
    auto barrier = vk::BufferMemoryBarrier2{};
    barrier.buffer = _resource;
    barrier.offset = 0;
    barrier.size = vk::WholeSize;
    barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.srcStageMask  = vk::PipelineStageFlagBits2::eComputeShader;
    barrier.srcAccessMask = vk::AccessFlagBits2::eShaderStorageWrite;
    barrier.dstStageMask  = vk::PipelineStageFlagBits2::eComputeShader;
    barrier.dstAccessMask = dstAccess;

    const auto dependency = vk::DependencyInfo{}.setBufferMemoryBarriers(barrier);
    cmd.pipelineBarrier2(dependency);
}

void tpd::StorageBuffer::recordComputeDstAccess(
    const std::vector<vk::Buffer>& buffers,
    const vk::CommandBuffer cmd,
    const vk::AccessFlags2 dstAccess) noexcept
{
    const auto toBarrier = [&dstAccess](const vk::Buffer buffer) {
        auto barrier = vk::BufferMemoryBarrier2{};
        barrier.buffer = buffer;
        barrier.offset = 0;
        barrier.size = vk::WholeSize;
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.srcStageMask  = vk::PipelineStageFlagBits2::eComputeShader;
        barrier.srcAccessMask = vk::AccessFlagBits2::eShaderStorageWrite;
        barrier.dstStageMask  = vk::PipelineStageFlagBits2::eComputeShader;
        barrier.dstAccessMask = dstAccess;
        return barrier;
    };
    const auto barriers = buffers | std::views::transform(toBarrier) | std::ranges::to<std::vector>();
    const auto dependency = vk::DependencyInfo{}.setBufferMemoryBarriers(barriers);
    cmd.pipelineBarrier2(dependency);
}

void tpd::StorageBuffer::recordComputeExecution(
    const std::vector<vk::Buffer>& buffers,
    const vk::CommandBuffer cmd) noexcept
{
    // A pipeline barrier/event without any access flags is an execution dependency
    const auto toBarrier = [](const vk::Buffer buffer) {
        auto barrier = vk::BufferMemoryBarrier2{};
        barrier.buffer = buffer;
        barrier.offset = 0;
        barrier.size = vk::WholeSize;
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.srcStageMask  = vk::PipelineStageFlagBits2::eComputeShader;
        barrier.dstStageMask  = vk::PipelineStageFlagBits2::eComputeShader;
        return barrier;
    };
    const auto barriers = buffers | std::views::transform(toBarrier) | std::ranges::to<std::vector>();
    const auto dependency = vk::DependencyInfo{}.setBufferMemoryBarriers(barriers);
    cmd.pipelineBarrier2(dependency);
}
