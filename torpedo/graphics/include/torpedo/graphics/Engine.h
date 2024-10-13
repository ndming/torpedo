#pragma once

#include "torpedo/foundation/ResourceAllocator.h"
#include "torpedo/graphics/Renderer.h"

namespace tpd {
    class Engine final {
    public:
        static std::unique_ptr<Engine> create(PFN_vkDebugUtilsMessengerCallbackEXT debugCallback = nullptr);

        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;

        std::unique_ptr<Renderer> createRenderer(RenderEngine renderEngine, void* nativeWindow = nullptr);
        void destroyRenderer(const std::unique_ptr<Renderer>& renderer);

        ~Engine();

    private:
        explicit Engine(PFN_vkDebugUtilsMessengerCallbackEXT debugCallback) noexcept;

        // Vulkan instance
        vk::Instance _instance{};
        void createInstance(PFN_vkDebugUtilsMessengerCallbackEXT debugCallback);
        static std::vector<const char*> getRequiredExtensions();

#ifndef NDEBUG
        // Debug messenger
        vk::DebugUtilsMessengerEXT _debugMessenger{};
        void createDebugMessenger(PFN_vkDebugUtilsMessengerCallbackEXT debugCallback);
#endif

        // Device and the transfer queue
        vk::Device _device{};
        vk::Queue _transferQueue{};

        // Resource allocator
        std::unique_ptr<ResourceAllocator> _allocator{};
        void initResourceAllocator(vk::PhysicalDevice physicalDevice);

        // Transfer command pool
        vk::CommandPool _transferCommandPool{};
        void createTransferCommandPool(uint32_t transferQueueFamily);

        // Transfer operations
        [[nodiscard]] vk::CommandBuffer beginSingleTimeTransferCommands() const;
        void endSingleTimeTransferCommands(vk::CommandBuffer commandBuffer) const;

        static constexpr std::initializer_list<const char*> ENGINE_DEVICE_EXTENSIONS = {
            vk::EXTMemoryBudgetExtensionName,    // help VMA estimate memory budget more accurately
            vk::EXTMemoryPriorityExtensionName,  // incorporate memory priority to the allocator
        };
    };
}
