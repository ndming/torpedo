#pragma once

#include "torpedo/rendering/Renderer.h"

#include <torpedo/foundation/VmaUsage.h>

namespace tpd {
    struct PhysicalDeviceSelection;
    class Engine;

    template<typename T>
    concept EngineImpl = std::is_base_of_v<Engine, T> && std::is_final_v<T>;

    class Engine {
    public:
        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;

        void waitIdle() const noexcept;

        virtual ~Engine() noexcept { Engine::destroy(); }

    protected:
        Engine() = default;
        void init(vk::Instance instance, vk::SurfaceKHR surface, std::vector<const char*>&& rendererDeviceExtensions);

        [[nodiscard]] virtual std::vector<const char*> getDeviceExtensions() const;

        [[nodiscard]] virtual PhysicalDeviceSelection pickPhysicalDevice(
            const std::vector<const char*>& deviceExtensions,
            vk::Instance instance, vk::SurfaceKHR surface) const;

        [[nodiscard]] virtual vk::Device createDevice(
            const std::vector<const char*>& deviceExtensions,
            std::initializer_list<uint32_t> queueFamilyIndices) const;

        [[nodiscard]] virtual const char* getName() const noexcept;
        [[nodiscard]] virtual std::pmr::memory_resource* getFrameResource() noexcept;

        virtual void onInitialized() {} // called by Context, not Engine base
        virtual void destroy() noexcept;

        /*--------------------*/

        Renderer* _renderer{ nullptr };
        bool _initialized{ false };
        vk::PhysicalDevice _physicalDevice{};
        vk::Device _device{};
        VmaAllocator _vmaAllocator{};

        uint32_t _graphicsFamilyIndex{};
        uint32_t _transferFamilyIndex{};
        uint32_t _computeFamilyIndex{};
        uint32_t _presentFamilyIndex{};

        /*--------------------*/

        template<RendererImpl R>
        friend class Context;
    };
} // namespace tpd

inline void tpd::Engine::waitIdle() const noexcept {
    _device.waitIdle();
}

inline const char* tpd::Engine::getName() const noexcept {
    return "tpd::Engine";
}

inline std::pmr::memory_resource* tpd::Engine::getFrameResource() noexcept {
    return std::pmr::get_default_resource();
}
