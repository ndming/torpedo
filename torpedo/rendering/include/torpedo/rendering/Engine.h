#pragma once

#include <torpedo/bootstrap/PhysicalDeviceSelector.h>
#include <torpedo/foundation/DeviceAllocator.h>

namespace tpd {
    class Renderer;

    class Engine {
    public:
        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;

        void init(Renderer& renderer);

        using DeviceAllocatorType = std::unique_ptr<DeviceAllocator, foundation::Deleter<DeviceAllocator>>;
        const DeviceAllocatorType& getDeviceAllocator() const noexcept;

        [[nodiscard]] virtual vk::CommandBuffer draw(vk::Image image) const = 0;

        virtual void destroy() noexcept;
        virtual ~Engine() noexcept;

    protected:
        Engine() = default;
        bool _initialized{ false };
        Renderer* _renderer{ nullptr };

        [[nodiscard]] virtual std::vector<const char*> getDeviceExtensions() const;

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

        std::pmr::unsynchronized_pool_resource _engineObjectPool{};
        DeviceAllocatorType _deviceAllocator{};

        static void transitionImageLayout(
            vk::CommandBuffer buffer,
            vk::Image image,
            vk::ImageLayout oldLayout,
            vk::ImageLayout newLayout);

        static void logExtensions(
            std::string_view extensionType,
            std::string_view className,
            const std::vector<const char*>& extensions = {});

        [[nodiscard]] virtual const char* getName() const noexcept;
    };

    template<typename  T>
    std::unique_ptr<T> createEngine() requires std::derived_from<T, Engine> && std::is_final_v<T> {
        return std::make_unique<T>();
    }
}

// =========================== //
// INLINE FUNCTION DEFINITIONS //
// =========================== //

inline const tpd::Engine::DeviceAllocatorType& tpd::Engine::getDeviceAllocator() const noexcept {
    return _deviceAllocator;
}

inline const char* tpd::Engine::getName() const noexcept {
    return "tpd::Engine";
}
