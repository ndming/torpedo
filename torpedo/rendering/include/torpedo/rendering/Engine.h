#pragma once

#include "torpedo/rendering/Renderer.h"
#include "torpedo/rendering/DeletionWorker.h"
#include "torpedo/rendering/SyncGroup.h"

#include <torpedo/bootstrap/PhysicalDeviceSelector.h>
#include <torpedo/foundation/StagingBuffer.h>
#include <torpedo/foundation/StorageBuffer.h>
#include <torpedo/foundation/Texture.h>

namespace tpd {
    class Engine;

    template<typename T>
    concept EngineImpl = std::is_base_of_v<Engine, T> && std::is_final_v<T>;

    class Engine {
    public:
        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;

        [[nodiscard]] const DeviceAllocator& getDeviceAllocator() const noexcept;

        [[nodiscard]] virtual vk::CommandBuffer draw(vk::Image image) const = 0;
        void waitIdle() const noexcept;

        virtual ~Engine() noexcept;

    protected:
        Engine() = default;
        Renderer* _renderer{ nullptr };

        void init(vk::Instance instance, vk::SurfaceKHR surface, std::vector<const char*>&& rendererDeviceExtensions);
        bool _initialized{ false };

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
        uint32_t _presentFamilyIndex{};

        vk::Queue _graphicsQueue{};
        vk::Queue _transferQueue{};
        vk::Queue _computeQueue{};

    private:
        void createDrawingCommandPool();
        vk::CommandPool _drawingCommandPool{};

        void createStartupCommandPools();
        vk::CommandPool _graphicsCommandPool{};
        vk::CommandPool _transferCommandPool{};

        std::pmr::unsynchronized_pool_resource _memoryPool{};
        std::pmr::unsynchronized_pool_resource _entityPool{};

        std::mutex _startupCommandPoolMutex{};
        DeletionWorker<StagingBuffer> _stagingDeletionQueue{ &_memoryPool, _startupCommandPoolMutex, "TransferCleanup" };

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
        virtual void onInitialized() {}

        virtual void destroy() noexcept;

    public:
        void sync(const StorageBuffer& storageBuffer);
        void sync(const SyncGroup<StorageBuffer>& group);

        static constexpr auto TEXTURE_FINAL_LAYOUT = vk::ImageLayout::eShaderReadOnlyOptimal;
        void sync(Texture& texture);
        void sync(const SyncGroup<Texture>& group);

        void syncAndGenMips(Texture& texture);
        void syncAndGenMips(const SyncGroup<Texture>& group);

    private:
        template<RendererImpl R>
        friend class Context;
    };
}

// =========================== //
// INLINE FUNCTION DEFINITIONS //
// =========================== //

inline const tpd::DeviceAllocator& tpd::Engine::getDeviceAllocator() const noexcept {
    return *_deviceAllocator;
}

inline void tpd::Engine::waitIdle() const noexcept {
    _device.waitIdle();
}

inline const char* tpd::Engine::getName() const noexcept {
    return "tpd::Engine";
}
