#pragma once

#include "torpedo/graphics/View.h"

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

        [[nodiscard]] virtual std::unique_ptr<View> createView() const = 0;

        virtual void setOnFramebufferResize(const std::function<void(uint32_t, uint32_t)>& callback) = 0;
        virtual void setOnFramebufferResize(std::function<void(uint32_t, uint32_t)>&& callback) noexcept = 0;
        [[nodiscard]] virtual float getFramebufferAspectRatio() const = 0;

        virtual void render(const std::unique_ptr<View>& view) = 0;
        virtual void render(const std::unique_ptr<View>& view, const std::function<void(uint32_t)>& onFrameReady) = 0;

        void waitIdle() const noexcept;

        [[nodiscard]] virtual vk::GraphicsPipelineCreateInfo getGraphicsPipelineInfo() const = 0;
        [[nodiscard]] vk::Device getVulkanDevice() const;

        static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

        virtual ~Renderer() = default;

    protected:
        Renderer() = default;
        virtual void onCreate(vk::Instance instance, std::initializer_list<const char*> engineExtensions);
        virtual void onInitialize();

        // Physical device and queue family indices
        vk::PhysicalDevice _physicalDevice{};
        uint32_t _graphicsQueueFamily{};
        virtual void pickPhysicalDevice(vk::Instance instance, std::initializer_list<const char*> engineExtensions);
        [[nodiscard]] virtual PhysicalDeviceSelector getPhysicalDeviceSelector(std::initializer_list<const char*> engineExtensions) const;

        // Device and the graphics queue
        vk::Device _device{};
        vk::Queue _graphicsQueue{};
        virtual void createDevice(std::initializer_list<const char*> engineExtensions);
        [[nodiscard]] std::vector<const char*> getDeviceExtensions(std::initializer_list<const char*> engineExtensions) const;
        [[nodiscard]] virtual std::vector<const char*> getRendererExtensions() const;

        virtual void onFeaturesRegister();
        template <typename T> void addFeature(const T& feature);
        [[nodiscard]] vk::PhysicalDeviceFeatures2 buildDeviceFeatures(const vk::PhysicalDeviceFeatures& features) const;

        // Drawing command pool and resource allocator
        vk::CommandPool _drawingCommandPool{};
        ResourceAllocator* _allocator{ nullptr };

        vk::SampleCountFlagBits _msaaSampleCount{ vk::SampleCountFlagBits::e4 };
        float _minSampleShading{ 0.0f };

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
        void setAllocator(ResourceAllocator* allocator);

        friend class Engine;
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

template<typename T>
void tpd::Renderer::addFeature(const T& feature) {
    const auto features = new T{ feature };
    _rendererFeatures.emplace_back(features, &features->pNext, [](void* it) { delete static_cast<T*>(it); });
}

inline vk::Device tpd::Renderer::getVulkanDevice() const {
    return _device;
}

inline void tpd::Renderer::setAllocator(ResourceAllocator* const allocator) {
    _allocator = allocator;
}
