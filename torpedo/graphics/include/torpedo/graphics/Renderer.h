#pragma once

#include <torpedo/foundation/ResourceAllocator.h>
#include <torpedo/bootstrap/PhysicalDeviceSelector.h>

#include <functional>

namespace tpd {
    enum class RenderEngine {
        Forward,
    };

    class Renderer {
    public:
        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        virtual void render() = 0;
        virtual void render(const std::function<void(uint32_t)>& onFrameReady) = 0;

        virtual ~Renderer() = default;

    protected:
        Renderer() = default;
        virtual void onCreate(vk::Instance instance, std::initializer_list<const char*> deviceExtensions);
        virtual void onInitialize();

        // Physical device and queue family indices
        vk::PhysicalDevice _physicalDevice{};
        uint32_t _graphicsQueueFamily{};
        virtual void pickPhysicalDevice(vk::Instance instance, std::initializer_list<const char*> deviceExtensions);
        [[nodiscard]] virtual PhysicalDeviceSelector getPhysicalDeviceSelector(std::initializer_list<const char*> deviceExtensions) const;

        // Device and the graphics queue
        vk::Device _device{};
        vk::Queue _graphicsQueue{};
        virtual void createDevice(std::initializer_list<const char*> deviceExtensions);
        [[nodiscard]] std::vector<const char*> getDeviceExtensions(std::initializer_list<const char*> deviceExtensions) const;
        [[nodiscard]] virtual std::vector<const char*> getRendererExtensions() const;

        virtual void onFeaturesRegister();
        [[nodiscard]] static vk::PhysicalDeviceFeatures getFeatures();
        [[nodiscard]] static vk::PhysicalDeviceVertexInputDynamicStateFeaturesEXT getVertexInputDynamicStateFeatures();
        [[nodiscard]] static vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT getExtendedDynamicState3Features();

        void addFeature(void* feature, void** next, std::function<void(void*)>&& deallocate);
        [[nodiscard]] vk::PhysicalDeviceFeatures2 buildDeviceFeatures() const;

        // Drawing command pool and resource allocator
        vk::CommandPool _drawingCommandPool{};
        ResourceAllocator* _allocator{ nullptr };

        vk::SampleCountFlagBits _colorAttachmentSampleCount{ vk::SampleCountFlagBits::e4 };
        vk::SampleCountFlagBits _depthAttachmentSampleCount{ vk::SampleCountFlagBits::e4 };

        virtual void onDestroy(vk::Instance instance) noexcept;

    private:
        struct RendererFeature {
            void* feature{ nullptr };
            void** next{ nullptr };
            std::function<void(void*)> deallocate{ nullptr };
        };
        std::vector<RendererFeature> _rendererFeatures{};
        void clearRendererFeatures() noexcept;

        void createDrawingCommandPool();
        void setAllocator(ResourceAllocator* const allocator) { _allocator = allocator; }

        friend class Engine;
    };
}
