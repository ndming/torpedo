#pragma once

#include "torpedo/foundation/VmaUsage.h"

#include <deque>
#include <functional>
#include <mutex>
#include <thread>

namespace tpd {
    class StorageBuffer;
    class Texture;

    class DeletionWorker final {
    public:
        DeletionWorker(vk::Device device, VmaAllocator vmaAllocator);

        DeletionWorker(const DeletionWorker&) = delete;
        DeletionWorker& operator=(const DeletionWorker&) = delete;

        void start();

        void submit(
            vk::Fence fence, vk::Buffer buffer, VmaAllocation allocation, vk::Semaphore semaphore = {},
            std::vector<std::pair<vk::CommandPool, vk::CommandBuffer>>&& commandBuffers = {});

        void waitEmpty();

        void shutdown() noexcept;
        ~DeletionWorker() noexcept { shutdown(); }

    private:
        std::function<void(std::string_view)> _statusUpdateCallback{ [](std::string_view) {} };

        vk::Device _device;
        VmaAllocator _vmaAllocator;

        void deletionWork();
        std::thread _workerThread;

        struct Task {
            vk::Fence fence;
            OpaqueResource<vk::Buffer> buffer;
            vk::Semaphore semaphore;
            std::vector<std::pair<vk::CommandPool, vk::CommandBuffer>> buffers;
        };
        std::deque<Task> _tasks{};
        bool _stopWorker{ false };

        std::mutex _queueMutex{};
        std::condition_variable _queueCondition{};

        // According to the spec, the command pool and command buffers involved in vkFreeCommandBuffers
        // must be externally synchronized. While the command buffers are no longer the interest of the
        // main thread, the command pool may still be used by the main thread, hence an extra lock for it
        std::mutex _commandPoolMutex{};
        friend class TransferWorker;
    };

    class TransferWorker final {
    public:
        TransferWorker(
            uint32_t transferFamily, uint32_t graphicsFamily, uint32_t computeFamily,
            vk::PhysicalDevice physicalDevice, vk::Device device, VmaAllocator vmaAllocator);

        TransferWorker(const TransferWorker&) = delete;
        TransferWorker& operator=(const TransferWorker&) = delete;

        void transfer(const void* data, vk::DeviceSize size, const StorageBuffer& buffer, uint32_t dstFamily, SyncPoint dstSync);

        void transfer(
            const void* data, vk::DeviceSize size, const Texture& texture, vk::Extent3D extent, uint32_t dstFamily,
            vk::ImageLayout dstLayout = vk::ImageLayout::eShaderReadOnlyOptimal, uint32_t mipLevel = 0);

        void transfer(
            const void* data, vk::DeviceSize size, const Texture& texture, vk::Extent3D extent, uint32_t dstFamily,
            uint32_t mipCount, vk::ImageLayout dstLayout = vk::ImageLayout::eShaderReadOnlyOptimal);

        void setStatusUpdateCallback(const std::function<void(std::string_view)>& callback) noexcept;
        void setStatusUpdateCallback(std::function<void(std::string_view)>&& callback) noexcept;

        void waitIdle() { _deletionWorker.waitEmpty(); }

        void destroy() noexcept;

    private:
        [[nodiscard]] vk::CommandPool getPool(uint32_t queueFamily) const;

        [[nodiscard]] vk::CommandBuffer beginTransfer(uint32_t queueFamily) const;
        void endTransfer(vk::CommandBuffer buffer, vk::Fence deletionFence) const;

        [[nodiscard]] vk::SemaphoreSubmitInfo createOwnershipSemaphoreInfo() const;
        void endRelease(vk::CommandBuffer buffer, const vk::SemaphoreSubmitInfo& semaphoreInfo) const;
        void endAcquire(vk::CommandBuffer buffer, const vk::SemaphoreSubmitInfo& semaphoreInfo, uint32_t dstFamily, vk::Fence deletionFence) const;

        vk::PhysicalDevice _physicalDevice;
        DeletionWorker _deletionWorker;

        uint32_t _transferFamily;
        uint32_t _graphicsFamily;
        uint32_t _computeFamily;

        vk::Queue _transferQueue;
        vk::Queue _graphicsQueue;
        vk::Queue _computeQueue;

        vk::CommandPool _releasePool;
        vk::CommandPool _graphicsAcquirePool;
        vk::CommandPool _computeAcquirePool;
    };
} // namespace tpd

inline void tpd::TransferWorker::setStatusUpdateCallback(const std::function<void(std::string_view)>& callback) noexcept {
    _deletionWorker._statusUpdateCallback = callback;
}

inline void tpd::TransferWorker::setStatusUpdateCallback(std::function<void(std::string_view)>&& callback) noexcept {
    _deletionWorker._statusUpdateCallback = std::move(callback);
}

inline vk::CommandPool tpd::TransferWorker::getPool(const uint32_t queueFamily) const {
    if (queueFamily == _transferFamily) {
        return _releasePool;
    }
    if (queueFamily == _computeFamily) {
        return _computeAcquirePool;
    }
    if (queueFamily == _graphicsFamily) {
        return _graphicsAcquirePool;
    }
    [[unlikely]]
    throw std::invalid_argument("TransferWorker - Unrecognized queue family for transfer command pool");
}
