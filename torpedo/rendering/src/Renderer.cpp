#include "torpedo/rendering/Renderer.h"
#include "torpedo/rendering/Utils.h"

std::vector<const char*> tpd::Renderer::getInstanceExtensions() const {
#ifndef NDEBUB
    auto extensions = std::vector {
        vk::EXTDebugUtilsExtensionName,  // for naming Vulkan objects
    };
    rendering::logExtensions("Instance", "tpd::Renderer", extensions);
    return extensions;
#else
    return {};
#endif
}

std::vector<const char*> tpd::Renderer::getDeviceExtensions() const {
    rendering::logExtensions("Device", "tpd::Renderer");
    return {};
}

void tpd::Renderer::resetEngine() noexcept {
    _engineInitialized = false;
    _device = nullptr;
    _physicalDevice = nullptr;
}

void tpd::Renderer::destroy() noexcept {
    resetEngine();  // redundant, but still call for completeness
    _initialized = false;
    _instance = nullptr;
}

tpd::Renderer::~Renderer() noexcept {
    Renderer::destroy();
}

