#pragma once

#include "torpedo/rendering/Camera.h"
#include "torpedo/rendering/Renderer.h"
#include "torpedo/rendering/Engine.h"
#include "torpedo/rendering/LogUtils.h"

#include <torpedo/bootstrap/InstanceBuilder.h>
#include <torpedo/foundation/Allocation.h>

namespace tpd {
    class SurfaceRenderer;
    class HeadlessRenderer;

    template<RendererImpl R>
    class Context final {
    public:
        [[nodiscard]] static std::unique_ptr<Context> create();

        Context(const Context&) = delete;
        Context& operator=(const Context&) = delete;

        [[nodiscard]] R* initRenderer(uint32_t frameWidth, uint32_t frameHeight);
        [[nodiscard]] R* initRenderer(bool fullscreen = false);

        template<EngineImpl E>
        [[nodiscard]] std::unique_ptr<E, Deleter<E>> bindEngine();

        template<EngineImpl E>
        void destroyEngine(std::unique_ptr<E, Deleter<E>> engine) noexcept;

        template<CameraImpl C>
        [[nodiscard]] std::unique_ptr<C, Deleter<C>> createCamera();

        template<CameraImpl C>
        void destroyCamera(std::unique_ptr<C, Deleter<C>> camera) noexcept;

        ~Context() noexcept;

    private:
        // Keep Context resources close on the heap
        static std::pmr::unsynchronized_pool_resource _contextResource;

        explicit Context(R* renderer);
        R* _renderer;

        void createInstance(std::vector<const char*>&& instanceExtensions);
        vk::Instance _instance{};

#ifndef NDEBUG
        void createDebugMessenger();
        vk::DebugUtilsMessengerEXT _debugMessenger{};
#endif

        void engineInitRenderer() const;
        Engine* _engine{ nullptr };
    };
}

template<tpd::RendererImpl R>
std::pmr::unsynchronized_pool_resource tpd::Context<R>::_contextResource{};

template<tpd::RendererImpl R>
std::unique_ptr<tpd::Context<R>> tpd::Context<R>::create() {
    void* alloc = _contextResource.allocate(sizeof(R), alignof(R));
    R* renderer = new (alloc) R{};

    const auto context = new Context{ renderer };
    return std::unique_ptr<Context>{ context };
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
        PLOGD << " - " << properties.deviceName.data() << ": " << utils::formatDriverVersion(properties.driverVersion);
    }
}

#ifndef NDEBUG
#include <torpedo/bootstrap/DebugUtils.h>

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
        default: break;
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

    using namespace utils;
    if (createDebugUtilsMessenger(_instance, &debugInfo, &_debugMessenger) == vk::Result::eErrorExtensionNotPresent) [[unlikely]] {
        throw std::runtime_error("Context - Failed to set up a debug messenger: the extension is not present");
    }
}
#endif // NDEBUG - Context::createDebugMessenger

template<tpd::RendererImpl R>
R* tpd::Context<R>::initRenderer(const uint32_t frameWidth, const uint32_t frameHeight) {
    if (_renderer->initialized()) [[unlikely]] {
        PLOGW << "Context - A Renderer has already been initialized with the current Context: "
                 "create a new Context if you want to have another Renderer, returning nullptr";
        return nullptr;
    }

    _renderer->_instance = _instance;
    _renderer->init(frameWidth, frameHeight);

    if (_engine) [[unlikely]] {
        engineInitRenderer();
    }
    return _renderer;
}

template<tpd::RendererImpl R>
R* tpd::Context<R>::initRenderer(const bool fullscreen) {
    if (_renderer->initialized()) [[unlikely]] {
        PLOGW << "Context - A Renderer has already been initialized with the current Context: "
                 "create a new Context if you want to have another Renderer, returning nullptr";
        return nullptr;
    }

    if (!_renderer->supportSurfaceRendering()) [[unlikely]] {
        PLOGE << "Context - Could NOT full-screen initialize a Renderer with no surface rendering support: "
                 "use the initRenderer(width, height) overload instead";
        throw std::runtime_error("Context - Failed to initialize a Renderer");
    }

    _renderer->_instance = _instance;
    _renderer->init(fullscreen);

    if (_engine) [[unlikely]] {
        engineInitRenderer();
    }
    return _renderer;
}

template<tpd::RendererImpl R>
template<tpd::EngineImpl E>
std::unique_ptr<E, tpd::Deleter<E>> tpd::Context<R>::bindEngine() {
    if (!_renderer->initialized() && _renderer->supportSurfaceRendering()) [[unlikely]] {
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

    void* mem = _contextResource.allocate(sizeof(E), alignof(E));
    E* engine = new (mem) E{};
    _engine   = engine;

    // Tell the Engine which Renderer should also clear their resources,
    // then let the Engine decide the creation of PhysicalDevice and Device
    _engine->_renderer = _renderer;
    _engine->init(_instance, _renderer->getVulkanSurface(), _renderer->getDeviceExtensions());

    // If the renderer has not been initialized yet, chances are the call site wants to defer the renderer creation to
    // the last minute when the Engine is ready to draw. Therefore, we skip Renderer's initialization here and leave it 
    // to initRenderer. This only ever happens for renderers without surface rendering support.
    if (_renderer->initialized()) {
        engineInitRenderer();
    }

    // Tell Engine's implementations to init their resources. This step should be called after Renderer::engineInit()
    // since downstream may query infos relating to the Renderer's resources (i.e. swap chain resolutions)
    _engine->onInitialized();

    // This means the Engine is ready to draw frames
    _engine->_initialized = true;

    return std::unique_ptr<E, Deleter<E>>(engine, Deleter<E>{ &_contextResource });
}

template<tpd::RendererImpl R>
void tpd::Context<R>::engineInitRenderer() const {
    // Inform the renderer about the selected Vulkan resources
    _renderer->_physicalDevice = _engine->_physicalDevice;
    _renderer->_device = _engine->_device;

    // It's the Renderer's turn to initialize its own rendering resources (e.g. swap chain)
    if constexpr (std::is_same_v<R, SurfaceRenderer>) {
        _renderer->engineInit(_engine->_graphicsFamilyIndex, _engine->_presentFamilyIndex, _engine->getFrameResource());
    } else if constexpr (std::is_same_v<R, HeadlessRenderer>) {
        _renderer->engineInit(_engine->_graphicsFamilyIndex, _engine->_transferFamilyIndex, _engine->getFrameResource());
    } else {
        throw std::runtime_error("Context - Unrecognized Renderer implementation!");
    }
    _renderer->_engineInitialized = true;
}

template<tpd::RendererImpl R>
template<tpd::EngineImpl E>
void tpd::Context<R>::destroyEngine(std::unique_ptr<E, Deleter<E>> engine) noexcept {
    if (!engine) [[unlikely]] {
        PLOGW << "Context - Destroying an Engine that has already been destroyed: haha?";
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
template<tpd::CameraImpl C>
std::unique_ptr<C, tpd::Deleter<C>> tpd::Context<R>::createCamera() {
    if (!_renderer->initialized()) [[unlikely]] {
        PLOGE << "Context - Please init the renderer before creating any Camera!";
        throw std::runtime_error("Context - Create Camera before initializing the associated Renderer");
    }

    const auto [w, h] = _renderer->getFramebufferSize();
    auto camera = alloc::make_unique<C>(&_contextResource, w, h);

    _renderer->addFramebufferResizeCallback(camera.get(), [](void* ptr, const uint32_t width, const uint32_t height) {
        const auto cam = static_cast<Camera*>(ptr);
        cam->onImageSizeChange(width, height);
    });

    return camera;
}

template<tpd::RendererImpl R>
template<tpd::CameraImpl C>
void tpd::Context<R>::destroyCamera(std::unique_ptr<C, Deleter<C>> camera) noexcept {
    if (!camera) [[unlikely]] {
        PLOGW << "Context - Destroying a Camera that has already been destroyed: haha?";
        return;
    }
    _renderer->removeFramebufferResizeCallback(camera.get());
    camera.reset();
}

template<tpd::RendererImpl R>
tpd::Context<R>::~Context() noexcept {
    _renderer->destroy(); // don't call delete here
    _contextResource.deallocate(_renderer, sizeof(R), alignof(R));
    _renderer = nullptr;
#ifndef NDEBUG
    utils::destroyDebugUtilsMessenger(_instance, _debugMessenger);
#endif
    _instance.destroy();
}
