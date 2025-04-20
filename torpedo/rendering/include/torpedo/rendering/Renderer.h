#pragma once

#include <vulkan/vulkan.hpp>

#include <functional>
#include <map>
#include <memory_resource>

namespace tpd {
    class Renderer;

    template<typename T>
    concept RendererImpl = std::is_base_of_v<Renderer, T> && std::is_final_v<T>;

    class Renderer {
    public:
        struct FrameSync {
            vk::Semaphore imageReady{};
            vk::Semaphore renderDone{};
            vk::Fence frameDrawFence{};
        };

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        [[nodiscard]] virtual vk::Extent2D getFramebufferSize() const noexcept = 0;
        [[nodiscard]] virtual uint32_t getInFlightFrameCount() const noexcept;
        [[nodiscard]] virtual bool supportSurfaceRendering() const noexcept;

        [[nodiscard]] virtual FrameSync getCurrentFrameSync() const noexcept = 0;
        [[nodiscard]] virtual uint32_t getCurrentFrameIndex() const noexcept;

        using ResizeCallback = std::function<void(void*, uint32_t, uint32_t)>;
        void addFramebufferResizeCallback(void* ptr, const ResizeCallback& callback);
        void addFramebufferResizeCallback(void* ptr, ResizeCallback&& callback);
        void removeFramebufferResizeCallback(void* ptr) noexcept;

        virtual void resetEngine() noexcept;
        virtual ~Renderer() noexcept;

    protected:
        [[nodiscard]] virtual std::vector<const char*> getInstanceExtensions() const;
        [[nodiscard]] virtual std::vector<const char*> getDeviceExtensions() const;

        Renderer() = default;

        virtual void init(uint32_t frameWidth, uint32_t frameHeight) = 0;
        [[nodiscard]] virtual bool initialized() const noexcept = 0;

        // Returns null if implementation doesn't support surface rendering
        [[nodiscard]] virtual vk::SurfaceKHR getVulkanSurface() const;

        virtual void engineInit(uint32_t graphicsFamily, uint32_t queueFamily, std::pmr::memory_resource* frameResource) = 0;
        virtual void destroy() noexcept;

        /*--------------------*/

        vk::Instance _instance{};
        std::map<void*, ResizeCallback> _framebufferResizeListeners{};
        bool _engineInitialized{ false }; // set by Context, reset by Renderer
        vk::PhysicalDevice _physicalDevice{};
        vk::Device _device{};

        /*--------------------*/

        template<RendererImpl R>
        friend class Context;
    };
} // namespace tpd

inline uint32_t tpd::Renderer::getInFlightFrameCount() const noexcept {
    return 1;
}

inline uint32_t tpd::Renderer::getCurrentFrameIndex() const noexcept {
    return 0;
}

inline bool tpd::Renderer::supportSurfaceRendering() const noexcept {
    return false;
}

inline vk::SurfaceKHR tpd::Renderer::getVulkanSurface() const {
    return {};
}
