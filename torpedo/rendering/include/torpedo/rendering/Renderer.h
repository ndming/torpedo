#pragma once

#include "torpedo/rendering/Engine.h"

namespace tpd {
    class Renderer {
    public:
        Renderer() = default;
        virtual void init(uint32_t framebufferWidth, uint32_t framebufferHeight) = 0;

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        [[nodiscard]] virtual vk::Extent2D getFramebufferSize() const = 0;
        [[nodiscard]] virtual uint32_t getInFlightFramesCount() const = 0;

        [[nodiscard]] virtual uint32_t getCurrentFrame() const;
        [[nodiscard]] vk::Instance getVulkanInstance() const;
        [[nodiscard]] virtual vk::SurfaceKHR getVulkanSurface() const;

        virtual void resetEngine() noexcept;
        virtual void reset() noexcept;

        virtual ~Renderer() noexcept;

    protected:
        void init();
        bool _initialized{ false };

        [[nodiscard]] virtual std::vector<const char*> getInstanceExtensions() const;
        vk::Instance _instance{};

        [[nodiscard]] virtual std::vector<const char*> getDeviceExtensions() const;
        virtual void engineInit(vk::Device device, const PhysicalDeviceSelection& physicalDeviceSelection);
        bool _engineInitialized{ false };

        vk::PhysicalDevice _physicalDevice{};
        vk::Device _device{};

    private:
        void createInstance(std::vector<const char*>&& instanceExtensions);

#ifndef NDEBUG
        void createDebugMessenger();
        vk::DebugUtilsMessengerEXT _debugMessenger{};
#endif

        friend void Engine::init(Renderer&);
    };
}

// =========================== //
// INLINE FUNCTION DEFINITIONS //
// =========================== //

inline uint32_t tpd::Renderer::getCurrentFrame() const {
    return 0;
}

inline vk::SurfaceKHR tpd::Renderer::getVulkanSurface() const {
    return {};
}
