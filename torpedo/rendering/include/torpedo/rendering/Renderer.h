#pragma once

#include "torpedo/rendering/Engine.h"

#include <concepts>
#include <type_traits>

namespace tpd {
    class Renderer;
    struct PhysicalDeviceSelection;

    template<typename  T>
    std::unique_ptr<T> createRenderer() requires std::derived_from<T, Renderer> && std::is_final_v<T>;

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

        static void logExtensions(
            std::string_view extensionType,
            std::string_view className,
            const std::vector<const char*>& extensions = {});

    private:
        void createInstance(std::vector<const char*>&& instanceExtensions);

#ifndef NDEBUG
        void createDebugMessenger();
        vk::DebugUtilsMessengerEXT _debugMessenger{};
#endif

        friend void Engine::init(Renderer&);
    };
}

// =====================================================================================================================
// TEMPLATE FUNCTION DEFINITIONS
// =====================================================================================================================

template<typename  T>
std::unique_ptr<T> tpd::createRenderer() requires std::derived_from<T, Renderer> && std::is_final_v<T> {
    auto renderer = std::make_unique<T>();
    return renderer;
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline uint32_t tpd::Renderer::getCurrentFrame() const {
    return 0;
}

inline vk::SurfaceKHR tpd::Renderer::getVulkanSurface() const {
    return {};
}
