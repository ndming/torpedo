#pragma once

#include "torpedo/graphics/Camera.h"
#include "torpedo/graphics/View.h"

#include <torpedo/bootstrap/PhysicalDeviceSelector.h>
#include <torpedo/foundation/ResourceAllocator.h>

#include <functional>
#include <type_traits>

namespace tpd {
    class Renderer;

    template<typename T>
    concept Renderable = std::is_base_of_v<Renderer, T> && std::is_final_v<T>;

    template<Renderable R>
    std::unique_ptr<R> createRenderer(void* nativeWindow = nullptr);

    class Renderer {
    public:
        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        [[nodiscard]] std::unique_ptr<View> createView() const;

        template<Projectable T = Camera>
        [[nodiscard]] std::shared_ptr<T> createCamera() const;

        [[nodiscard]] virtual std::pair<uint32_t, uint32_t> getFramebufferSize() const = 0;

        virtual void render(const View& view, const std::function<void(uint32_t)>& onFrameReady) = 0;

        void copyBuffer(vk::Buffer src, vk::Buffer dst, const vk::BufferCopy& copyInfo) const;

        void waitIdle() const noexcept;

        [[nodiscard]] vk::Device getVulkanDevice() const;
        [[nodiscard]] const ResourceAllocator& getResourceAllocator() const;

        [[nodiscard]] virtual vk::GraphicsPipelineCreateInfo getGraphicsPipelineInfo() const = 0;

        static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

        virtual ~Renderer();

    protected:
        explicit Renderer(std::vector<const char*>&& requiredExtensions);
        vk::Instance _instance{};

        virtual void init();

        virtual void pickPhysicalDevice() = 0;
        vk::PhysicalDevice _physicalDevice{};
        uint32_t _graphicsQueueFamily{};

        virtual void createDevice() = 0;
        static std::vector<const char*> getDeviceExtensions();
        vk::Device _device{};
        vk::Queue _graphicsQueue{};

        virtual void registerFeatures();
        template <typename T> void addFeature(const T& feature);
        [[nodiscard]] vk::PhysicalDeviceFeatures2 buildDeviceFeatures(const vk::PhysicalDeviceFeatures& features) const;

        std::unique_ptr<ResourceAllocator> _allocator{};

        vk::CommandPool _drawingCommandPool{};
        vk::CommandPool _oneShotCommandPool{};

        vk::SampleCountFlagBits _msaaSampleCount{ vk::SampleCountFlagBits::e4 };
        float _minSampleShading{ 0.0f };

    private:
        void createInstance(std::vector<const char*>&& requiredExtensions);

#ifndef NDEBUG
        // Debug messenger
        vk::DebugUtilsMessengerEXT _debugMessenger{};
        void createDebugMessenger();
#endif

        struct RendererFeature {
            void* feature{ nullptr };
            void** next{ nullptr };
            std::function<void(void*)> deallocate{ nullptr };
        };
        std::vector<RendererFeature> _rendererFeatures{};
        void clearRendererFeatures() noexcept;

        void createResourceAllocator();

        void createDrawingCommandPool();
        void createOneShotCommandPool();

        void createSharedDescriptorSetLayout() const;
        void createSharedObjectBuffers() const;
        void writeSharedDescriptorSets() const;

        [[nodiscard]] vk::CommandBuffer beginOneShotCommands() const;
        void endOneShotCommands(vk::CommandBuffer commandBuffer) const;

        template<Renderable R>
        friend std::unique_ptr<R> createRenderer(void* nativeWindow) {
            auto renderer = std::make_unique<R>(nativeWindow);
            renderer->init();
            return renderer;
        }
    };
}

// =====================================================================================================================
// TEMPLATE FUNCTION DEFINITIONS
// =====================================================================================================================

template<tpd::Projectable T>
std::shared_ptr<T> tpd::Renderer::createCamera() const {
    const auto [width, height] = getFramebufferSize();
    return std::make_shared<T>(width, height);
}

template<typename T>
void tpd::Renderer::addFeature(const T& feature) {
    const auto features = new T{ feature };
    _rendererFeatures.emplace_back(features, &features->pNext, [](void* it) { delete static_cast<T*>(it); });
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline vk::Device tpd::Renderer::getVulkanDevice() const {
    return _device;
}

inline const tpd::ResourceAllocator& tpd::Renderer::getResourceAllocator() const {
    return *_allocator;
}
