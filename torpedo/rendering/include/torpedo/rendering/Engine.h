#pragma once

#include <vulkan/vulkan.hpp>

namespace tpd {
    class Renderer;
    class Engine;
    struct PhysicalDeviceSelection;

    template<typename  T>
    std::unique_ptr<T> createEngine() requires std::derived_from<T, Engine> && std::is_final_v<T>;

    class Engine {
    public:
        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;

        void init(Renderer& renderer);

        [[nodiscard]] virtual vk::CommandBuffer draw(vk::Image image) const = 0;

        virtual void destroy() noexcept;
        virtual ~Engine() noexcept;

    protected:
        Engine() = default;
        bool _initialized{ false };
        Renderer* _renderer{ nullptr };

        [[nodiscard]] virtual std::vector<const char*> getDeviceExtensions() const = 0;
        [[nodiscard]] virtual PhysicalDeviceSelection pickPhysicalDevice(
            const std::vector<const char*>& deviceExtensions,
            vk::Instance instance, vk::SurfaceKHR surface) const = 0;
        [[nodiscard]] virtual vk::Device createDevice(
            const std::vector<const char*>& deviceExtensions,
            std::initializer_list<uint32_t> queueFamilyIndices) const = 0;
        vk::PhysicalDevice _physicalDevice{};
        vk::Device _device{};

        vk::Queue _graphicsQueue{};
        vk::Queue _transferQueue{};
        vk::Queue _computeQueue{};

        void createDrawingCommandPool(uint32_t graphicsFamilyIndex);
        vk::CommandPool _drawingCommandPool{};

        void createDrawingCommandBuffers();
        std::vector<vk::CommandBuffer> _drawingCommandBuffers{};

        static void transitionImageLayout(
            vk::CommandBuffer buffer,
            vk::Image image,
            vk::ImageLayout oldLayout,
            vk::ImageLayout newLayout);

        static void logExtensions(
            std::string_view extensionType,
            std::string_view className,
            const std::vector<const char*>& extensions = {});

        [[nodiscard]] virtual const char* getName() const;
    };
}

// =====================================================================================================================
// TEMPLATE FUNCTION DEFINITIONS
// =====================================================================================================================

template<typename  T>
std::unique_ptr<T> tpd::createEngine() requires std::derived_from<T, Engine> && std::is_final_v<T> {
    return std::make_unique<T>();
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline const char* tpd::Engine::getName() const {
    return "tpd::Engine";
}
