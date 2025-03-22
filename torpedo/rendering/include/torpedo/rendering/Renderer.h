#pragma once

#include <vulkan/vulkan.hpp>

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

        virtual void resetEngine() noexcept;
        virtual ~Renderer() noexcept;

    protected:
        [[nodiscard]] virtual std::vector<const char*> getInstanceExtensions() const;
        [[nodiscard]] virtual std::vector<const char*> getDeviceExtensions() const;

        Renderer() = default;
        vk::Instance _instance{};

        virtual void init(uint32_t frameWidth, uint32_t frameHeight, std::pmr::memory_resource* contextPool = nullptr) = 0;
        bool _initialized{ false };

        [[nodiscard]] virtual vk::SurfaceKHR getVulkanSurface() const;

        virtual void engineInit(uint32_t graphicsFamilyIndex, uint32_t presentFamilyIndex) = 0;
        bool _engineInitialized{ false };

        vk::PhysicalDevice _physicalDevice{};
        vk::Device _device{};

        virtual void destroy() noexcept;

    private:
        template<RendererImpl R>
        friend class Context;
    };
}

// =========================== //
// INLINE FUNCTION DEFINITIONS //
// =========================== //

inline uint32_t tpd::Renderer::getInFlightFramesCount() const noexcept {
    return 1;
}

inline uint32_t tpd::Renderer::getCurrentDrawingFrame() const noexcept {
    return 0;
}

inline vk::SurfaceKHR tpd::Renderer::getVulkanSurface() const {
    return {};
}
