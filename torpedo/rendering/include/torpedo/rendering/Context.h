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

        [[nodiscard]] R* initRenderer(uint32_t frameWidth, uint32_t frameHeight);
        [[nodiscard]] R* initRenderer(bool fullscreen = false);

        template<EngineImpl E>
        [[nodiscard]] std::unique_ptr<E, Deleter<E>> bindEngine();

        template<EngineImpl E>
        void destroyEngine(std::unique_ptr<E, Deleter<E>> engine) noexcept;

        ~Context() noexcept;

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
        .applicationVersion(0, 0, 0)
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
R* tpd::Context<R>::initRenderer(const uint32_t frameWidth, const uint32_t frameHeight) {
    if (_renderer->_initialized) [[unlikely]] {
        PLOGW << "Context - A Renderer has already been initialized with the current Context: "
                 "create a new Context if you want to have another Renderer, returning nullptr";
        return nullptr;
    }

    _renderer->_instance = _instance;
    _renderer->init(frameWidth, frameHeight, &_contextPool);
    _renderer->_initialized = true;

    // If there's already an Engine bound to this Context, init the Renderer with its Vulkan resources
    if (_engine) {
        _renderer->_physicalDevice = _engine->_physicalDevice;
        _renderer->_device = _engine->_device;

        _renderer->engineInit(_engine->_graphicsFamilyIndex, _engine->_presentFamilyIndex);
        _renderer->_engineInitialized = true;
    }

    return _renderer;
}

template<tpd::RendererImpl R>
R* tpd::Context<R>::initRenderer(const bool fullscreen) {
    if (_renderer->_initialized) [[unlikely]] {
        PLOGW << "Context - A Renderer has already been initialized with the current Context: "
                 "create a new Context if you want to have another Renderer, returning nullptr";
        return nullptr;
    }

    _renderer->_instance = _instance;
    _renderer->init(fullscreen, &_contextPool);
    _renderer->_initialized = true;

    // If there's already an Engine bound to this Context, init the Renderer with its Vulkan resources
    if (_engine) {
        _renderer->_physicalDevice = _engine->_physicalDevice;
        _renderer->_device = _engine->_device;

        _renderer->engineInit(_engine->_graphicsFamilyIndex, _engine->_presentFamilyIndex);
        _renderer->_engineInitialized = true;
    }

    return _renderer;
}

template<tpd::RendererImpl R>
template<tpd::EngineImpl E>
std::unique_ptr<E, tpd::Deleter<E>> tpd::Context<R>::bindEngine() {
    if (!_renderer->_initialized && _renderer->hasSurfaceRenderingSupport()) [[unlikely]] {
        PLOGE << "Context - Danger! Binding an Engine while the associated Renderer has not been initialized: "
                 "the renderer type has surface support, call Context::initRenderer() prior to Engine binding";
        throw std::runtime_error("Context - Renderer must be initialized before binding an Engine with surface support");
    }

    if (_engine && typeid(E) == typeid(*_engine)) [[unlikely]] {
        PLOGW << "Context - An Engine of the same type has already been bound with the current Context: returning null";
        return nullptr;
    }

    if (_engine) [[unlikely]] {
        PLOGW << "Context - Binding to a different Engine while the previously bound one is still alive: "
                 "all draw commands of the old Engine may result in undefined behavior, proceed with care";
        // Detach the Renderer from the previously bound Engine
        _renderer->resetEngine();
        _engine->_renderer = nullptr;
        _engine = nullptr;
    }

    void* mem = _contextPool.allocate(sizeof(E), alignof(E));
    E* engine = new (mem) E{};
    _engine   = engine;

    // Tell the Engine which Renderer should also clear their resources,
    // then let the Engine decide the creation of PhysicalDevice and Device
    _engine->_renderer = _renderer;
    _engine->init(_instance, _renderer->getVulkanSurface(), _renderer->getDeviceExtensions());

    // If the renderer has not been initialized yet, chances are the call site wants to defer the renderer creation to
    // the last minute when the Engine is ready to draw. Therefore, we skip Renderer's initialization here and leave it 
    // to initRenderer. This only ever happens for renderers without surface rendering support.
    if (_renderer->_initialized) {
        // Inform the renderer about the selected Vulkan resources
        _renderer->_physicalDevice = _engine->_physicalDevice;
        _renderer->_device = _engine->_device;

        // It's the Renderer's turn to initialize its own rendering resources (e.g. swap chain)
        _renderer->engineInit(_engine->_graphicsFamilyIndex, _engine->_presentFamilyIndex);
        _renderer->_engineInitialized = true;
    }

    // This means the Engine is ready to draw frames
    _engine->_initialized = true;

    // Tell Engine's implementations to init their resources. This step should be called after Renderer::engineInit()
    // since downstream may query infos relating to the Renderer's resources (i.e. swap chain resolutions)
    _engine->onInitialized();

    return std::unique_ptr<E, Deleter<E>>(engine, Deleter<E>{ &_contextPool });
}

template<tpd::RendererImpl R>
template<tpd::EngineImpl E>
void tpd::Context<R>::destroyEngine(std::unique_ptr<E, Deleter<E>> engine) noexcept {
    if (!engine) [[unlikely]] {
        PLOGW << "Context - Destroying an Engine that has likely already been destroyed: is this a joke?";
        return;
    }
    if (!_engine || _engine != engine.get()) [[unlikely]] {
        PLOGW << "Context - Destroying an Engine that has not been bound to this Context: "
                 "a Context can only destroy an Engine that was previously bound to it";
        return;
    }

    engine.reset();
    _engine = nullptr;
}

template<tpd::RendererImpl R>
tpd::Context<R>::~Context() noexcept {
    _renderer->destroy();  // don't call delete here
    _contextPool.deallocate(_renderer, sizeof(R), alignof(R));
}
