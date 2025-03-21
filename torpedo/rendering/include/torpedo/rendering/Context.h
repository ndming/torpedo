#pragma once

#include "torpedo/rendering/Renderer.h"
#include "torpedo/rendering/Engine.h"
#include "torpedo/rendering/Utils.h"

#include <torpedo/bootstrap/InstanceBuilder.h>
#include <torpedo/bootstrap/DebugUtils.h>

namespace tpd {
    template<RendererImpl R>
    class Context final {
    public:
        [[nodiscard]] static std::unique_ptr<Context, Deleter<Context>> create();

        Context(const Context&) = delete;
        Context& operator=(const Context&) = delete;

        [[nodiscard]] std::unique_ptr<R, Deleter<R>> initRenderer(uint32_t frameWidth, uint32_t frameHeight);
        [[nodiscard]] std::unique_ptr<R, Deleter<R>> initRenderer(bool fullscreen = false);

        template<EngineImpl E>
        [[nodiscard]] std::unique_ptr<E, Deleter<E>> bindEngine();

    private:
        // Keep Context, Renderer, Engine, and their resources close on the heap
        static std::pmr::unsynchronized_pool_resource _contextPool;

        explicit Context(R* renderer);
        R* _renderer;
        Engine* _engine{ nullptr };

        void createInstance(std::vector<const char*>&& instanceExtensions);
        vk::Instance _instance{};

#ifndef NDEBUG
        void createDebugMessenger();
        vk::DebugUtilsMessengerEXT _debugMessenger{};
#endif

        vk::PhysicalDevice _physicalDevice{};
        vk::Device _device{};
    };

    namespace rendering {
        template<RendererImpl R>
        constexpr auto FullScreenInitType = requires(R obj, bool fullscreen, std::pmr::memory_resource* contextPool) {
            { obj.init(fullscreen, contextPool) };
        };
    }
}

template<tpd::RendererImpl R>
std::pmr::unsynchronized_pool_resource tpd::Context<R>::_contextPool{};

template<tpd::RendererImpl R>
std::unique_ptr<tpd::Context<R>, tpd::Deleter<tpd::Context<R>>> tpd::Context<R>::create() {
    void* alloc = _contextPool.allocate(sizeof(R), alignof(R));
    R* renderer = new (alloc) R{};

    void* mem = _contextPool.allocate(sizeof(Context), alignof(Context));
    const auto context = new (mem) Context{ renderer };
    return std::unique_ptr<Context, Deleter<Context>>(context, Deleter<Context>{ &_contextPool });
}

template<tpd::RendererImpl R>
tpd::Context<R>::Context(R* renderer) : _renderer{ renderer } {
    createInstance(_renderer->getInstanceExtensions());
#ifndef NDEBUG
    createDebugMessenger();
#endif
    const auto devices = _instance.enumeratePhysicalDevices();
    PLOGI << "Found a working Vulkan instance";

    PLOGD << "Available devices (" << devices.size() << "):";
    for (const auto& device : devices) {
        const auto properties = device.getProperties();
        PLOGD << " - " << properties.deviceName.data() << ": " << rendering::formatDriverVersion(properties.driverVersion);
    }
}

#ifndef NDEBUG
VKAPI_ATTR inline vk::Bool32 VKAPI_CALL torpedoDebugMessengerCallback(
    const vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    [[maybe_unused]] const vk::DebugUtilsMessageTypeFlagsEXT messageType,
    const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
    [[maybe_unused]] void* const userData)
{
    using enum vk::DebugUtilsMessageSeverityFlagBitsEXT;
    switch (messageSeverity) {
    case eVerbose: PLOGV << pCallbackData->pMessage; break;
    case eInfo:    PLOGI << pCallbackData->pMessage; break;
    case eWarning: PLOGW << pCallbackData->pMessage; break;
    case eError:   PLOGE << pCallbackData->pMessage; throw std::runtime_error("Vulkan in panic!");
    default:
    }
    return vk::False;
}
#endif // NDEBUG - torpedoDebugMessengerCallback

template<tpd::RendererImpl R>
void tpd::Context<R>::createInstance(std::vector<const char*>&& instanceExtensions) {
    auto instanceCreateFlags = vk::InstanceCreateFlags{};

#ifdef __APPLE__
    // Beginning with the 1.3.216 Vulkan SDK, the VK_KHR_PORTABILITY_subset extension is mandatory
    // for macOS with the latest MoltenVK SDK
    requiredExtensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
    instanceCreateFlags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif

    _instance = InstanceBuilder()
        .applicationVersion(1, 0, 0)
        .apiVersion(1, 3, 0)
        .extensions(std::move(instanceExtensions))
#ifndef NDEBUG
        .debugInfoCallback(torpedoDebugMessengerCallback)
        .build(instanceCreateFlags, { "VK_LAYER_KHRONOS_validation" });
#else
        .build(instanceCreateFlags);
#endif

    PLOGI << "Using Vulkan API version: 1.3";
}

#ifndef NDEBUG
template<tpd::RendererImpl R>
void tpd::Context<R>::createDebugMessenger() {
    using MessageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT;
    using MessageType = vk::DebugUtilsMessageTypeFlagBitsEXT;

    auto debugInfo = vk::DebugUtilsMessengerCreateInfoEXT{};
    debugInfo.messageSeverity = MessageSeverity::eVerbose | MessageSeverity::eWarning | MessageSeverity::eError;
    debugInfo.messageType = MessageType::eGeneral | MessageType::eValidation | MessageType::ePerformance;
    debugInfo.pfnUserCallback = torpedoDebugMessengerCallback;

    using namespace bootstrap;
    if (createDebugUtilsMessenger(_instance, &debugInfo, &_debugMessenger) == vk::Result::eErrorExtensionNotPresent) [[unlikely]] {
        throw std::runtime_error("Context - Failed to set up a debug messenger: the extension is not present");
    }
}
#endif // NDEBUG - Context::createDebugMessenger

template<tpd::RendererImpl R>
std::unique_ptr<R, tpd::Deleter<R>> tpd::Context<R>::initRenderer(const uint32_t frameWidth, const uint32_t frameHeight) {
    if (_renderer->_initialized) [[unlikely]] {
        PLOGW << "Context - A Renderer has already been initialized with the current Context: "
                 "create a new Context if you want to have another Renderer, returning nullptr";
        return nullptr;
    }

    _renderer->_instance = _instance;
    _renderer->init(frameWidth, frameHeight, &_contextPool);
    _renderer->_initialized = true;

    return std::unique_ptr<R, Deleter<R>>(_renderer, Deleter<R>{ &_contextPool });
}

template<tpd::RendererImpl R>
std::unique_ptr<R, tpd::Deleter<R>> tpd::Context<R>::initRenderer(const bool fullscreen) {
    if (_renderer->_initialized) [[unlikely]] {
        PLOGW << "Context - A Renderer has already been initialized with the current Context: "
                 "create a new Context if you want to have another Renderer, returning nullptr";
        return nullptr;
    }

    _renderer->_instance = _instance;
    _renderer->init(fullscreen, &_contextPool);
    _renderer->_initialized = true;

    return std::unique_ptr<R, Deleter<R>>(_renderer, Deleter<R>{ &_contextPool });
}

template<tpd::RendererImpl R>
template<tpd::EngineImpl E>
std::unique_ptr<E, tpd::Deleter<E>> tpd::Context<R>::bindEngine() {
    if (!_renderer->_initialized) [[unlikely]] {
        PLOGE << "Context - Binding an Engine without having initialized the Context's associated renderer: "
                 "due to the way Vulkan's backend works, init a Renderer first using Context::initRenderer()";
        throw std::runtime_error("Context - Failed to bind an engine: uninitialized Renderer");
    }

    if (_engine && typeid(E) == typeid(*_engine)) [[unlikely]] {
        PLOGW << "Context - An Engine of the same type has already been bound with the current Context: returning null";
        return nullptr;
    }

    if (_engine) {
        _engine->destroy();  // safe to call multiple times
        _engine = nullptr;
    }

    void* mem = _contextPool.allocate(sizeof(E), alignof(E));
    E* engine = new (mem) E{};
    _engine   = engine;

    // Tell the Engine which Renderer should also clear their resources,
    // then let the Engine decide the creation of PhysicalDevice and Device
    _engine->_renderer = _renderer;
    _engine->init(_instance, _renderer->getVulkanSurface(), _renderer->getDeviceExtensions());

    // Inform the renderer about the selected Vulkan resources, then tell the Engine
    // which Renderer should also clear their resources in the event the Engine is destroyed
    _renderer->_physicalDevice = _engine->_physicalDevice;
    _renderer->_device = _engine->_device;

    // It's the Renderer's turn to initialize its own rendering resources (e.g. swap chain)
    _renderer->engineInit(_engine->_graphicsFamilyIndex, _engine->_presentFamilyIndex);
    _renderer->_engineInitialized = true;

    // This means all Vulkan and Engine resources have been initialized,
    // and the associated Renderer has initialized their Vulkan objects.
    _engine->_initialized = true;

    return std::unique_ptr<E, Deleter<E>>(engine, Deleter<E>{ &_contextPool });
}
