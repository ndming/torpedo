#include "torpedo/rendering/TransferWorker.h"

#include <torpedo/foundation/StorageBuffer.h>
#include <torpedo/foundation/Texture.h>

#include <plog/Log.h>

tpd::DeletionWorker::DeletionWorker(const vk::Device device, VmaAllocator vmaAllocator)
    : _device{ device }, _vmaAllocator{ vmaAllocator }
    , _workerThread{ std::thread(&DeletionWorker::deletionWork, this) }
{
    PLOGD << "DeletionWorker - Launched 1 deletion thread";
}

void tpd::DeletionWorker::submit(
    const vk::Fence fence,
    const vk::Buffer buffer,
    VmaAllocation allocation,
    const vk::Semaphore semaphore,
    std::vector<std::pair<vk::CommandPool, vk::CommandBuffer>>&& commandBuffers)
{
    {
        std::lock_guard lock(_queueMutex);
        auto resource = OpaqueResource{ buffer, allocation };
        PLOGD << "DeletionWorker - Inserting a resource: " << resource.toString();
        _tasks.emplace_back(fence, std::move(resource), semaphore, std::move(commandBuffers));
    }
    _queueCondition.notify_one();
}

void tpd::DeletionWorker::deletionWork() {
    while (true) {
        // Sleep until there a deletion task to work on, or we're shutting down
        std::unique_lock lock(_queueMutex);
        _queueCondition.wait(lock, [this] { return !_tasks.empty() || _stopWorker; });

        if (_stopWorker && _tasks.empty()) break;

        auto [fence, resource, semaphore, buffers] = std::move(_tasks.front());
        _tasks.pop_front();
        lock.unlock(); // shared-data has been read, we can unlock to let other threads add more tasks

        // Wait until the GPU is done with the resource then destroy it
        using limits = std::numeric_limits<uint64_t>;
        // The fence has been submitted by the main thread prior to this point, so no sync needed
        [[maybe_unused]] const auto result = _device.waitForFences(fence, vk::True, limits::max());
        // VMA is thread-safe internally
        const auto resName = resource.toString();
        resource.destroy(_vmaAllocator);

        // The destruction of semaphore and fence are thread-safe
        _device.destroyFence(fence);
        if (semaphore) _device.destroySemaphore(semaphore);

        {
            // VkCommandPool in vkFreeCommandBuffers must be synchronized externally
            // The same is applied to buffers, but the main thread no longer uses them
            std::lock_guard poolLock(_commandPoolMutex);
            for (const auto [pool, buffer] : buffers) _device.freeCommandBuffers(pool, buffer);
        }

        PLOGD << "DeletionWorker - Destroyed a resource: " << resName;

        // Notify the main thread who could potentially be waiting until all tasks are free
        // If that's not the case, this signal can be lost without causing any issue
        _queueCondition.notify_one();
    }
}

void tpd::DeletionWorker::waitEmpty() {
    if (_workerThread.joinable()) {
        std::unique_lock lock(_queueMutex);
        _queueCondition.wait(lock, [this] { return _tasks.empty(); });
    }
}

void tpd::DeletionWorker::shutdown() noexcept {
    if (_workerThread.joinable()) {
        {
            std::lock_guard lock(_queueMutex);
            _stopWorker = true;
        }
        _queueCondition.notify_one();
        _workerThread.join();
        PLOGD << "DeletionWorker - Shut down 1 deletion thread";
    }
}

tpd::TransferWorker::TransferWorker(
    const uint32_t transferFamily,
    const uint32_t graphicsFamily,
    const uint32_t computeFamily,
    const vk::PhysicalDevice physicalDevice,
    const vk::Device device,
    VmaAllocator vmaAllocator)
    : _physicalDevice{ physicalDevice }
    , _deletionWorker{ device, vmaAllocator }
    , _transferFamily{ transferFamily }
    , _graphicsFamily{ graphicsFamily }
    , _computeFamily{ computeFamily }
{
    _transferQueue = device.getQueue(transferFamily, 0);
    _graphicsQueue = device.getQueue(graphicsFamily, 0);
    _computeQueue  = device.getQueue(computeFamily, 0);

    _releasePool = device.createCommandPool({ vk::CommandPoolCreateFlagBits::eTransient, transferFamily });
    _computeAcquirePool  = device.createCommandPool({ vk::CommandPoolCreateFlagBits::eTransient, computeFamily  });
    _graphicsAcquirePool = device.createCommandPool({ vk::CommandPoolCreateFlagBits::eTransient, graphicsFamily });
}

void tpd::TransferWorker::transfer(
    const void* data,
    const vk::DeviceSize size,
    const StorageBuffer& buffer,
    const uint32_t dstFamily,
    const SyncPoint dstSync)
{
    if (size == 0) {
        return;
    }

    const auto [stagingBuffer, stagingAllocation] = vma::allocateStagingBuffer(_deletionWorker._vmaAllocator, size);
    vma::copyStagingData(_deletionWorker._vmaAllocator, data, size, stagingAllocation);

    const auto releaseCommand = beginTransfer(_transferFamily);
    buffer.recordStagingCopy(releaseCommand, stagingBuffer, size);

    // Create a fence that will be signaled once all queued operations are completed.
    // This allows the deletion worker thread to safely clean up resources after ensuring
    // that no GPU operations are still using them.
    const auto deletionFence = _deletionWorker._device.createFence(vk::FenceCreateInfo{});

    if (_transferFamily != dstFamily) {
        constexpr auto srcSync = SyncPoint{ vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite };
        buffer.recordOwnershipRelease(releaseCommand, _transferFamily, dstFamily, srcSync);

        const auto ownershipSemaphoreInfo = createOwnershipSemaphoreInfo();
        endRelease(releaseCommand, ownershipSemaphoreInfo);

        const auto acquireCommand = beginTransfer(dstFamily);
        buffer.recordOwnershipAcquire(acquireCommand, _transferFamily, dstFamily, dstSync);
        endAcquire(acquireCommand, ownershipSemaphoreInfo, dstFamily, deletionFence);

        _deletionWorker.submit(
            deletionFence, stagingBuffer, stagingAllocation, ownershipSemaphoreInfo.semaphore,
            { { getPool(dstFamily), acquireCommand }, { _releasePool, releaseCommand } });
    } else {
        // Ensure subsequent commands don't access the buffer during copy
        buffer.recordTransferDstPoint(releaseCommand, dstSync);
        endTransfer(releaseCommand, deletionFence);
        _deletionWorker.submit(deletionFence, stagingBuffer, stagingAllocation, {}, {{ _releasePool, releaseCommand }});
    }
}

void tpd::TransferWorker::transfer(
    const void* data,
    const vk::DeviceSize size,
    const Texture& texture,
    const vk::Extent3D extent,
    const uint32_t dstFamily,
    const vk::ImageLayout dstLayout,
    const uint32_t mipLevel)
{
    if (size == 0) {
        return;
    }

    const auto [stagingBuffer, stagingAllocation] = vma::allocateStagingBuffer(_deletionWorker._vmaAllocator, size);
    vma::copyStagingData(_deletionWorker._vmaAllocator, data, size, stagingAllocation);

    const auto releaseCommand = beginTransfer(_transferFamily);
    texture.recordLayoutTransition(releaseCommand, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, mipLevel, 1);
    texture.recordStagingCopy(releaseCommand, stagingBuffer, extent, mipLevel);

    // Create a fence that will be signaled once all queued operations are completed.
    // This allows the deletion worker thread to safely clean up resources after ensuring
    // that no GPU operations are still using them.
    const auto deletionFence = _deletionWorker._device.createFence(vk::FenceCreateInfo{});

    if (_transferFamily != dstFamily) {
        constexpr auto srcSync = SyncPoint{ vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite };
        texture.recordOwnershipRelease(
            releaseCommand, _transferFamily, dstFamily, srcSync, vk::ImageLayout::eTransferDstOptimal, dstLayout, mipLevel, 1);

        const auto ownershipSemaphoreInfo = createOwnershipSemaphoreInfo();
        endRelease(releaseCommand, ownershipSemaphoreInfo);

        const auto acquireCommand = beginTransfer(dstFamily);

        const auto dstSync = texture.getLayoutTransitionDstSync(dstLayout);
        texture.recordOwnershipAcquire(
            acquireCommand, _transferFamily, dstFamily, dstSync, vk::ImageLayout::eTransferDstOptimal, dstLayout, mipLevel, 1);

        endAcquire(acquireCommand, ownershipSemaphoreInfo, dstFamily, deletionFence);
        _deletionWorker.submit(
            deletionFence, stagingBuffer, stagingAllocation, ownershipSemaphoreInfo.semaphore,
            { { getPool(dstFamily), acquireCommand }, { _releasePool, releaseCommand } });

    } else {
        texture.recordLayoutTransition(releaseCommand, vk::ImageLayout::eTransferDstOptimal, dstLayout, mipLevel, 1);
        endTransfer(releaseCommand, deletionFence);
        _deletionWorker.submit(deletionFence, stagingBuffer, stagingAllocation, {}, {{ _releasePool, releaseCommand }});
    }
}

void tpd::TransferWorker::transfer(
    const void* data,
    const vk::DeviceSize size,
    const Texture& texture,
    const vk::Extent3D extent,
    const uint32_t dstFamily,
    const uint32_t mipCount,
    const vk::ImageLayout dstLayout)
{
    if (size == 0) {
        return;
    }

    const auto [stagingBuffer, stagingAllocation] = vma::allocateStagingBuffer(_deletionWorker._vmaAllocator, size);
    vma::copyStagingData(_deletionWorker._vmaAllocator, data, size, stagingAllocation);

    const auto releaseCommand = beginTransfer(_transferFamily);
    // Transition all mip levels to transfer dst, not just the base mip
    texture.recordLayoutTransition(releaseCommand, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
    // Copy data to the base mip
    texture.recordStagingCopy(releaseCommand, stagingBuffer, extent);

    // Create a fence that will be signaled once all queued operations are completed.
    // This allows the deletion worker thread to safely clean up resources after ensuring
    // that no GPU operations are still using them.
    const auto deletionFence = _deletionWorker._device.createFence(vk::FenceCreateInfo{});

    if (_transferFamily != dstFamily) {
        texture.recordReleaseForMipGen(releaseCommand, _transferFamily, _graphicsFamily);

        const auto ownershipSemaphoreInfo = createOwnershipSemaphoreInfo();
        endRelease(releaseCommand, ownershipSemaphoreInfo);

        const auto acquireCommand = beginTransfer(dstFamily);
        texture.recordAcquireForMipGen(acquireCommand, _transferFamily, _graphicsFamily);
        texture.recordMipGen(acquireCommand, _physicalDevice, { extent.width, extent.height }, mipCount, dstLayout);

        endAcquire(acquireCommand, ownershipSemaphoreInfo, dstFamily, deletionFence);
        _deletionWorker.submit(
            deletionFence, stagingBuffer, stagingAllocation, ownershipSemaphoreInfo.semaphore,
            { { getPool(dstFamily), acquireCommand }, { _releasePool, releaseCommand } });
    } else {
        texture.recordMipGen(releaseCommand, _physicalDevice, { extent.width, extent.height }, mipCount, dstLayout);
        endTransfer(releaseCommand, deletionFence);
        _deletionWorker.submit(deletionFence, stagingBuffer, stagingAllocation, {}, {{ _releasePool, releaseCommand }});
    }
}

vk::CommandBuffer tpd::TransferWorker::beginTransfer(const uint32_t queueFamily) {
    const auto allocInfo = vk::CommandBufferAllocateInfo{}
        .setCommandPool(getPool(queueFamily))
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(1);

    // Ensure thread-safe access in case the deletion worker is using the command pool
    std::lock_guard lock(_deletionWorker._commandPoolMutex);
    const auto buffer = _deletionWorker._device.allocateCommandBuffers(allocInfo)[0];

    constexpr auto beginInfo = vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
    buffer.begin(beginInfo);

    return buffer;
}

void tpd::TransferWorker::endTransfer(const vk::CommandBuffer buffer, const vk::Fence deletionFence) const {
    buffer.end();
    _transferQueue.submit(vk::SubmitInfo{ {}, {}, {}, 1, &buffer }, deletionFence);
}

vk::SemaphoreSubmitInfo tpd::TransferWorker::createOwnershipSemaphoreInfo() const {
    const auto ownershipTransferSemaphore = _deletionWorker._device.createSemaphore({});
    auto semaphoreInfo = vk::SemaphoreSubmitInfo{};
    semaphoreInfo.semaphore = ownershipTransferSemaphore;
    // The only valid wait stage for ownership transfer is all-command
    semaphoreInfo.stageMask = vk::PipelineStageFlagBits2::eAllCommands;
    semaphoreInfo.deviceIndex = 0;
    semaphoreInfo.value = 1;
    return semaphoreInfo;
}

void tpd::TransferWorker::endRelease(const vk::CommandBuffer buffer, const vk::SemaphoreSubmitInfo& semaphoreInfo) const {
    buffer.end();

    auto releaseInfo = vk::CommandBufferSubmitInfo{};
    releaseInfo.commandBuffer = buffer;
    releaseInfo.deviceMask = 0b1;  // ignored by single-GPU setups

    const auto releaseSubmitInfo = vk::SubmitInfo2{}
        .setCommandBufferInfos(releaseInfo)
        .setSignalSemaphoreInfos(semaphoreInfo);
    _transferQueue.submit2(releaseSubmitInfo);
}

void tpd::TransferWorker::endAcquire(
    const vk::CommandBuffer buffer,
    const vk::SemaphoreSubmitInfo& semaphoreInfo,
    const uint32_t dstFamily,
    const vk::Fence deletionFence) const
{
    buffer.end();

    auto acquireInfo = vk::CommandBufferSubmitInfo{};
    acquireInfo.commandBuffer = buffer;
    acquireInfo.deviceMask = 0b1; // ignored by single-GPU setups

    const auto acquireSubmitInfo = vk::SubmitInfo2{}
        .setCommandBufferInfos(acquireInfo)
        .setWaitSemaphoreInfos(semaphoreInfo);

    if (dstFamily == _graphicsFamily) {
        _graphicsQueue.submit2(acquireSubmitInfo, deletionFence);
    } else if (dstFamily == _computeFamily) {
        _computeQueue.submit2(acquireSubmitInfo, deletionFence);
    } else [[unlikely]] {
        throw std::invalid_argument("TransferWorker - Acquire queue family must be either compute or graphics");
    }
}

void tpd::TransferWorker::destroy() noexcept {
    waitIdle();

    _deletionWorker._device.destroyCommandPool(_graphicsAcquirePool);
    _deletionWorker._device.destroyCommandPool(_computeAcquirePool);
    _deletionWorker._device.destroyCommandPool(_releasePool);
}
