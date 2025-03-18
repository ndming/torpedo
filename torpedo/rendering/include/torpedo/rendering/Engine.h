#pragma once

#include "torpedo/rendering/DeletionWorker.h"
#include "torpedo/rendering/SyncGroup.h"

#include <torpedo/bootstrap/PhysicalDeviceSelector.h>
#include <torpedo/foundation/StagingBuffer.h>
#include <torpedo/foundation/StorageBuffer.h>
#include <torpedo/foundation/Texture.h>

namespace tpd {
    class Renderer;

    class Engine {
    public:
        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;

        void init(Renderer& renderer);

        [[nodiscard]] const DeviceAllocator& getDeviceAllocator() const noexcept;

        [[nodiscard]] virtual vk::CommandBuffer draw(vk::Image image) const = 0;

        virtual void destroy() noexcept;
        virtual ~Engine() noexcept;

    protected:
        Engine() = default;
        bool _initialized{ false };
        Renderer* _renderer{ nullptr };

        [[nodiscard]] virtual std::vector<const char*> getDeviceExtensions() const;

        [[nodiscard]] virtual PhysicalDeviceSelection pickPhysicalDevice(
            const std::vector<const char*>& deviceExtensions,
            vk::Instance instance, vk::SurfaceKHR surface) const = 0;

        [[nodiscard]] virtual vk::Device createDevice(
            const std::vector<const char*>& deviceExtensions,
            std::initializer_list<uint32_t> queueFamilyIndices) const = 0;

        vk::PhysicalDevice _physicalDevice{};
        vk::Device _device{};

        uint32_t _graphicsFamilyIndex{};
        uint32_t _transferFamilyIndex{};
        uint32_t _computeFamilyIndex{};

        vk::Queue _graphicsQueue{};
        vk::Queue _transferQueue{};
        vk::Queue _computeQueue{};

    private:
        void createDrawingCommandPool();
        vk::CommandPool _drawingCommandPool{};

        void createStartupCommandPools();
        vk::CommandPool _graphicsCommandPool{};
        vk::CommandPool _transferCommandPool{};

        std::pmr::unsynchronized_pool_resource _engineObjectPool{};
        std::mutex _startupCommandPoolMutex{};

        DeletionWorker<StagingBuffer> _stagingDeletionQueue{ &_engineObjectPool, _startupCommandPoolMutex, "TransferCleanup" };

    protected:
        void createDrawingCommandBuffers();
        std::vector<vk::CommandBuffer> _drawingCommandBuffers{};

        [[nodiscard]] vk::CommandBuffer beginOneTimeTransfer(vk::CommandPool commandPool);
        void endOneTimeTransfer(vk::CommandBuffer buffer, vk::Fence deletionFence) const;

        [[nodiscard]] vk::SemaphoreSubmitInfo createOwnershipSemaphoreInfo() const;
        void endReleaseCommands(vk::CommandBuffer buffer, const vk::SemaphoreSubmitInfo& semaphoreInfo) const;
        void endAcquireCommands(vk::CommandBuffer buffer, const vk::SemaphoreSubmitInfo& semaphoreInfo, vk::Fence deletionFence) const;

        using DeviceAllocatorType = std::unique_ptr<DeviceAllocator, Deleter<DeviceAllocator>>;
        DeviceAllocatorType _deviceAllocator{};

        [[nodiscard]] virtual const char* getName() const noexcept;

    public:
        void sync(const StorageBuffer& storageBuffer);
        void sync(const SyncGroup<StorageBuffer>& group);

        static constexpr auto TEXTURE_FINAL_LAYOUT = vk::ImageLayout::eShaderReadOnlyOptimal;
        void sync(Texture& texture);
        void sync(const SyncGroup<Texture>& group);

        void syncAndGenMips(Texture& texture);
        void syncAndGenMips(const SyncGroup<Texture>& group);
    };
}

// =========================== //
// INLINE FUNCTION DEFINITIONS //
// =========================== //

inline const tpd::DeviceAllocator& tpd::Engine::getDeviceAllocator() const noexcept {
    return *_deviceAllocator;
}

inline const char* tpd::Engine::getName() const noexcept {
    return "tpd::Engine";
}
