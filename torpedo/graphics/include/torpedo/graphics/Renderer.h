#pragma once

#include "torpedo/graphics/Camera.h"
#include "torpedo/graphics/View.h"

#include <torpedo/bootstrap/PhysicalDeviceSelector.h>
#include <torpedo/foundation/Image.h>
#include <torpedo/foundation/ResourceAllocator.h>
#include <torpedo/foundation/ShaderLayout.h>

#include <functional>
#include <type_traits>

namespace tpd {
    class Renderer;

    template<typename T>
    concept Renderable = std::is_base_of_v<Renderer, T> && std::is_final_v<T>;

    template<Renderable R>
    std::unique_ptr<R> createRenderer();

    class Renderer {
    public:
        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        [[nodiscard]] std::unique_ptr<View> createView() const;

        template<Projectable T = Camera>
        [[nodiscard]] std::shared_ptr<T> createCamera() const;

        [[nodiscard]] virtual std::pair<uint32_t, uint32_t> getFramebufferSize() const = 0;

        virtual void render(const View& view, const std::function<void(uint32_t)>& onFrameReady) = 0;

        void copyBufferToBuffer(vk::Buffer src, vk::Buffer dst, const vk::BufferCopy& copyInfo) const;
        void copyBufferToImage(vk::Buffer buffer, vk::Image image, const vk::BufferImageCopy& copyInfo) const;

        void transitionImageLayout(vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::Image image) const;

        void waitIdle() const noexcept;

        [[nodiscard]] vk::Device getVulkanDevice() const;
        [[nodiscard]] const ResourceAllocator& getResourceAllocator() const;
        [[nodiscard]] std::size_t getMinUniformBufferOffsetAlignment() const;
        [[nodiscard]] float getMaxSamplerAnisotropy() const;

        [[nodiscard]] virtual vk::GraphicsPipelineCreateInfo getGraphicsPipelineInfo() const;
        [[nodiscard]] virtual vk::GraphicsPipelineCreateInfo getGraphicsPipelineInfo(float minSampleShading) const = 0;

        [[nodiscard]] const std::unique_ptr<Image>& getDefaultShaderReadImage() const;
        [[nodiscard]] vk::Sampler getDefaultSampler() const;

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

        std::unique_ptr<ShaderInstance> _sharedShaderInstance{};

        std::unique_ptr<Buffer> _drawableObjectBuffer{};
        std::unique_ptr<Buffer> _cameraObjectBuffer{};
        std::unique_ptr<Buffer> _lightObjectBuffer{};

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

        void createSharedDescriptorSetLayout();
        std::unique_ptr<ShaderLayout> _sharedShaderLayout{};

        void createSharedObjectBuffers();
        void writeSharedDescriptorSets() const;

        void createDefaultResources();
        std::unique_ptr<Image> _defaultShaderReadImage{};
        vk::Sampler _defaultSampler{};

        [[nodiscard]] vk::CommandBuffer beginOneShotCommands() const;
        void endOneShotCommands(vk::CommandBuffer commandBuffer) const;

        template<Renderable R>
        friend std::unique_ptr<R> createRenderer() {
            auto renderer = std::make_unique<R>();
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

inline std::size_t tpd::Renderer::getMinUniformBufferOffsetAlignment() const {
    return _physicalDevice.getProperties().limits.minUniformBufferOffsetAlignment;
}

inline float tpd::Renderer::getMaxSamplerAnisotropy() const {
    return _physicalDevice.getProperties().limits.maxSamplerAnisotropy;
}

inline vk::GraphicsPipelineCreateInfo tpd::Renderer::getGraphicsPipelineInfo() const {
    return getGraphicsPipelineInfo(0.0f);
}

inline const std::unique_ptr<tpd::Image>& tpd::Renderer::getDefaultShaderReadImage() const {
    return _defaultShaderReadImage;
}

inline vk::Sampler tpd::Renderer::getDefaultSampler() const {
    return _defaultSampler;
}
