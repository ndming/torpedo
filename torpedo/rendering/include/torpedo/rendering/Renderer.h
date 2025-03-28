#pragma once

#include <vulkan/vulkan.hpp>

#include <functional>

namespace tpd {
    class Renderer;

    template<typename T>
    concept RendererImpl = std::is_base_of_v<Renderer, T> && std::is_final_v<T>;

    class Renderer {
    public:
        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        [[nodiscard]] virtual vk::Extent2D getFramebufferSize() const noexcept = 0;
        [[nodiscard]] virtual uint32_t getInFlightFramesCount() const noexcept;
        [[nodiscard]] virtual uint32_t getCurrentDrawingFrame() const noexcept;
        [[nodiscard]] virtual bool hasSurfaceRenderingSupport() const noexcept;

        void addFramebufferResizeCallback(void* ptr, const std::function<void(void*, uint32_t, uint32_t)>& callback);
        void addFramebufferResizeCallback(void* ptr, std::function<void(void*, uint32_t, uint32_t)>&& callback);
        void removeFramebufferResizeCallback(void* ptr) noexcept;

        virtual void resetEngine() noexcept;
        virtual ~Renderer() noexcept;

    protected:
        [[nodiscard]] virtual std::vector<const char*> getInstanceExtensions() const;
        [[nodiscard]] virtual std::vector<const char*> getDeviceExtensions() const;

        Renderer() = default;
        vk::Instance _instance{};

        virtual void init(uint32_t frameWidth, uint32_t frameHeight, std::pmr::memory_resource* contextPool) = 0;
        bool _initialized{ false };

        // Init base class's resources
        void init(std::pmr::memory_resource* contextPool);
        std::pmr::unordered_map<void*, std::function<void(void*, uint32_t, uint32_t)>> _framebufferResizeListeners{};

        [[nodiscard]] virtual vk::SurfaceKHR getVulkanSurface() const;

        virtual void engineInit(uint32_t graphicsFamilyIndex, uint32_t presentFamilyIndex) = 0;
        bool _engineInitialized{ false };

        vk::PhysicalDevice _physicalDevice{};
        vk::Device _device{};

        virtual void destroy() noexcept;

        template<RendererImpl R>
        friend class Context;
    };
}  // namespace tpd

inline uint32_t tpd::Renderer::getInFlightFramesCount() const noexcept {
    return 1;
}

inline uint32_t tpd::Renderer::getCurrentDrawingFrame() const noexcept {
    return 0;
}

inline bool tpd::Renderer::hasSurfaceRenderingSupport() const noexcept {
    return false;
}

inline vk::SurfaceKHR tpd::Renderer::getVulkanSurface() const {
    return {};
}

// ========================= //
// UTILITY MACRO DEFINITIONS //
// ========================= //

#define TPD_CONTEXT_AWARE_INITIALIZATION(ClassName) \
    template<RendererImpl R>                        \
    friend class Context;
