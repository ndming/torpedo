#pragma once

#include "torpedo/graphics/Camera.h"
#include "torpedo/graphics/View.h"

#include <torpedo/bootstrap/PhysicalDeviceSelector.h>
#include <torpedo/foundation/ResourceAllocator.h>
#include <torpedo/foundation/ShaderLayout.h>

#include <functional>

namespace tpd {
    enum class RenderEngine {
        Forward,
    };

    class Renderer {
    public:
        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        [[nodiscard]] std::unique_ptr<View> createView() const;

        template<Projectable T = Camera>
        [[nodiscard]] std::shared_ptr<T> createCamera() const;

        virtual void setOnFramebufferResize(const std::function<void(uint32_t, uint32_t)>& callback) = 0;
        virtual void setOnFramebufferResize(std::function<void(uint32_t, uint32_t)>&& callback) noexcept = 0;
        [[nodiscard]] virtual std::pair<uint32_t, uint32_t> getFramebufferSize() const = 0;

        virtual void render(const View& view) = 0;
        virtual void render(const View& view, const std::function<void(uint32_t)>& onFrameReady) = 0;

        void waitIdle() const noexcept;

        [[nodiscard]] virtual vk::GraphicsPipelineCreateInfo getGraphicsPipelineInfo() const = 0;
        [[nodiscard]] vk::Device getVulkanDevice() const;

        [[nodiscard]] static ShaderLayout::Builder getSharedDescriptorLayoutBuilder(uint32_t setCount = 0);

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

        const ResourceAllocator* _allocator{ nullptr };

        vk::CommandPool _drawingCommandPool{};

        // A ShaderInstance holding one shared descriptor set for each in-flight frames
        std::unique_ptr<ShaderInstance> _sharedDescriptorSets{};

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

        void createSharedDescriptorSetLayout();
        void createSharedObjectBuffers() const;
        void writeSharedDescriptorSets() const;

        std::unique_ptr<ShaderLayout> _sharedDescriptorSetLayout{};

        void setAllocator(const ResourceAllocator* allocator);

        friend class Engine;
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
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

inline void tpd::Renderer::setAllocator(const ResourceAllocator* const allocator) {
    _allocator = allocator;
}

inline vk::Device tpd::Renderer::getVulkanDevice() const {
    return _device;
}

inline tpd::ShaderLayout::Builder tpd::Renderer::getSharedDescriptorLayoutBuilder(const uint32_t setCount) {
    return ShaderLayout::Builder()
        .descriptorSetCount(setCount + 1)
        .descriptor(0, 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex);
}
