#include "torpedo/foundation/Buffer.h"

void tpd::Buffer::recordOwnershipRelease(
    const vk::CommandBuffer cmd,
    const uint32_t srcFamily,
    const uint32_t dstFamily,
    const SyncPoint srcSync) const noexcept
{
    auto barrier = vk::BufferMemoryBarrier2{};
    barrier.buffer = _resource;
    barrier.offset = 0;
    barrier.size   = vk::WholeSize;
    barrier.srcQueueFamilyIndex = srcFamily;
    barrier.dstQueueFamilyIndex = dstFamily;
    barrier.srcStageMask  = srcSync.stage;
    barrier.srcAccessMask = srcSync.access;

    // TODO: consider VK_DEPENDENCY_QUEUE_FAMILY_OWNERSHIP_TRANSFER_USE_ALL_STAGES_BIT_KHR to avoid full pipeline stalls
    const auto dependency = vk::DependencyInfo{}.setBufferMemoryBarriers(barrier);
    cmd.pipelineBarrier2(dependency);
}

void tpd::Buffer::recordOwnershipAcquire(
    const vk::CommandBuffer cmd,
    const uint32_t srcFamily,
    const uint32_t dstFamily,
    const SyncPoint dstSync) const noexcept
{
    auto barrier = vk::BufferMemoryBarrier2{};
    barrier.buffer = _resource;
    barrier.offset = 0;
    barrier.size   = vk::WholeSize;
    barrier.srcQueueFamilyIndex = srcFamily;
    barrier.dstQueueFamilyIndex = dstFamily;
    barrier.dstStageMask  = dstSync.stage;
    barrier.dstAccessMask = dstSync.access;

    // TODO: consider VK_DEPENDENCY_QUEUE_FAMILY_OWNERSHIP_TRANSFER_USE_ALL_STAGES_BIT_KHR to avoid full pipeline stalls
    const auto dependency = vk::DependencyInfo{}.setBufferMemoryBarriers(barrier);
    cmd.pipelineBarrier2(dependency);
}
