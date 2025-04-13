#include "torpedo/rendering/Renderer.h"
#include "torpedo/rendering/LogUtils.h"

std::vector<const char*> tpd::Renderer::getInstanceExtensions() const {
#ifndef NDEBUB
    auto extensions = std::vector {
        vk::EXTDebugUtilsExtensionName,  // for naming Vulkan objects
    };
    rendering::logDebugExtensions("Instance", "tpd::Renderer", extensions);
    return extensions;
#else
    return {};
#endif
}

std::vector<const char*> tpd::Renderer::getDeviceExtensions() const {
    rendering::logDebugExtensions("Device", "tpd::Renderer");
    return {};
}

void tpd::Renderer::init(std::pmr::memory_resource* contextPool) {
    _framebufferResizeListeners = std::pmr::unordered_map<void*, std::function<void(void*, uint32_t, uint32_t)>>{ contextPool };
    _initialized = true;
    PLOGD << "Initialized base renderer: tpd::Renderer";
}

void tpd::Renderer::addFramebufferResizeCallback(void* ptr, const std::function<void(void*, uint32_t, uint32_t)>& callback) {
    _framebufferResizeListeners[ptr] = callback;
}

void tpd::Renderer::addFramebufferResizeCallback(void* ptr, std::function<void(void*, uint32_t, uint32_t)>&& callback) {
    _framebufferResizeListeners[ptr] = std::move(callback);
}

void tpd::Renderer::removeFramebufferResizeCallback(void* ptr) noexcept {
    _framebufferResizeListeners.erase(ptr);
}

void tpd::Renderer::resetEngine() noexcept {
    _engineInitialized = false;
    _device = nullptr;
    _physicalDevice = nullptr;
}

void tpd::Renderer::destroy() noexcept {
    resetEngine();  // redundant, but still call for completeness
    _initialized = false;
    _framebufferResizeListeners.clear();
    _instance = nullptr;
}

tpd::Renderer::~Renderer() noexcept {
    Renderer::destroy();
}

