#pragma once

#include "torpedo/rendering/Renderer.h"
#include "torpedo/rendering/DeletionWorker.h"
#include "torpedo/rendering/SyncGroup.h"

#include <torpedo/bootstrap/PhysicalDeviceSelector.h>
#include <torpedo/foundation/StagingBuffer.h>
#include <torpedo/foundation/StorageBuffer.h>
#include <torpedo/foundation/Texture.h>

#include <memory_resource>

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
            /// Additional semaphores to wait on (e.g. for async compute)
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
        void createSyncWorkCommandPools();
        std::mutex _syncWorkPoolMutex{};
        vk::CommandPool _transferPool{};
        vk::CommandPool _graphicsPool{};
        vk::CommandPool _computePool{};

        std::pmr::synchronized_pool_resource _syncResourcePool{};
        DeletionWorker<StagingBuffer> _stagingDeletionQueue{ &_syncResourcePool, _syncWorkPoolMutex, "TransferCleanup" };

    protected:
        // Keep drawing resources close together
        std::pmr::unsynchronized_pool_resource _engineResourcePool{};
        DeviceAllocator _deviceAllocator{};

        void createDrawingCommandPool();
        vk::CommandPool _drawingCommandPool{};

        void createDrawingCommandBuffers();
        std::pmr::vector<vk::CommandBuffer> _drawingCommandBuffers{ &_engineResourcePool };

        [[nodiscard]] virtual const char* getName() const noexcept;
        virtual void onInitialized() {}  // called by Context, not Engine base

        virtual void destroy() noexcept;

    private:
        [[nodiscard]] vk::CommandPool getSyncPool(uint32_t queueFamily) const;
        [[nodiscard]] vk::CommandBuffer beginSyncTransfer(uint32_t queueFamily);
        void endSyncCommands(vk::CommandBuffer buffer, vk::Fence deletionFence) const;

        [[nodiscard]] vk::SemaphoreSubmitInfo createSyncOwnershipSemaphoreInfo() const;
        void endSyncReleaseCommands(vk::CommandBuffer buffer, const vk::SemaphoreSubmitInfo& semaphoreInfo) const;
        void endSyncAcquireCommands(
            vk::CommandBuffer buffer, const vk::SemaphoreSubmitInfo& semaphoreInfo,
            uint32_t acquireFamily, vk::Fence deletionFence) const;

    protected:
        void sync(const StorageBuffer& storageBuffer, uint32_t acquireFamily);
        void sync(const SyncGroup<StorageBuffer>& group);

        static constexpr auto TEXTURE_FINAL_LAYOUT = vk::ImageLayout::eShaderReadOnlyOptimal;
        void sync(Texture& texture, uint32_t acquireFamily);
        void sync(const SyncGroup<Texture>& group);

        void syncAndGenMips(Texture& texture, uint32_t acquireFamily);
        void syncAndGenMips(const SyncGroup<Texture>& group);

        template<RendererImpl R>
        friend class Context;
    };
} // namespace tpd

inline void tpd::Engine::waitIdle() const noexcept {
    _device.waitIdle();
}

inline const char* tpd::Engine::getName() const noexcept {
    return "tpd::Engine";
}
