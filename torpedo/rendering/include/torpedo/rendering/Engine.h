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

        struct DrawPackage {
            /// Primary draw commands for graphics queue submit
            vk::CommandBuffer buffer;
            /// At which stage to wait for image from the renderer
            vk::PipelineStageFlags2 waitStage;
            /// At which stage to signal the renderer that the draw is done
            vk::PipelineStageFlags2 doneStage;
            // Additional semaphores to wait on (e.g. for async compute)
            std::vector<std::pair<vk::Semaphore, vk::PipelineStageFlags2>> waits{};
        };

        [[nodiscard]] virtual DrawPackage draw(vk::Image image) = 0;

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
            vk::Instance instance, vk::SurfaceKHR surface) const;

        [[nodiscard]] virtual vk::Device createDevice(
            const std::vector<const char*>& deviceExtensions,
            std::initializer_list<uint32_t> queueFamilyIndices) const;

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

        void createSyncWorkCommandPools();
        std::mutex _syncWorkCommandPoolMutex{};
        vk::CommandPool _graphicsCommandPool{};
        vk::CommandPool _transferCommandPool{};

        std::pmr::synchronized_pool_resource _syncResourcePool{};
        DeletionWorker<StagingBuffer> _stagingDeletionQueue{ &_syncResourcePool, _syncWorkCommandPoolMutex, "TransferCleanup" };

    protected:
        // Keep drawing resources close together
        std::pmr::unsynchronized_pool_resource _engineResourcePool{};
        
        void createDrawingCommandBuffers();
        std::pmr::vector<vk::CommandBuffer> _drawingCommandBuffers{ &_engineResourcePool };

        using DeviceAllocatorType = std::unique_ptr<DeviceAllocator, Deleter<DeviceAllocator>>;
        DeviceAllocatorType _deviceAllocator{};  // must be declared after _syncResourcePool

        [[nodiscard]] virtual const char* getName() const noexcept;
        virtual void onInitialized() {}  // called by Context, not Engine base

        virtual void destroy() noexcept;

        [[nodiscard]] vk::CommandBuffer beginOneTimeTransfer(vk::CommandPool commandPool);
        void endOneTimeTransfer(vk::CommandBuffer buffer, vk::Fence deletionFence) const;

        [[nodiscard]] vk::SemaphoreSubmitInfo createSyncOwnershipSemaphoreInfo() const;
        void endSyncReleaseCommands(vk::CommandBuffer buffer, const vk::SemaphoreSubmitInfo& semaphoreInfo) const;
        void endSyncAcquireCommands(vk::CommandBuffer buffer, const vk::SemaphoreSubmitInfo& semaphoreInfo, vk::Fence deletionFence) const;

        void sync(const StorageBuffer& storageBuffer);
        void sync(const SyncGroup<StorageBuffer>& group);

        static constexpr auto TEXTURE_FINAL_LAYOUT = vk::ImageLayout::eShaderReadOnlyOptimal;
        void sync(Texture& texture);
        void sync(const SyncGroup<Texture>& group);

        void syncAndGenMips(Texture& texture);
        void syncAndGenMips(const SyncGroup<Texture>& group);

        template<RendererImpl R>
        friend class Context;
    };
}  // namespace tpd

inline void tpd::Engine::waitIdle() const noexcept {
    _device.waitIdle();
}

inline const char* tpd::Engine::getName() const noexcept {
    return "tpd::Engine";
}
