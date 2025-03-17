#include "torpedo/rendering/Renderer.h"
#include "torpedo/rendering/Utils.h"

#include <torpedo/bootstrap/DeviceBuilder.h>
#include <torpedo/bootstrap/InstanceBuilder.h>
#include <torpedo/bootstrap/PhysicalDeviceSelector.h>
#include <torpedo/bootstrap/DebugUtils.h>

#include <plog/Log.h>

void tpd::Renderer::init() {
    createInstance(getInstanceExtensions());
#ifndef NDEBUG
    createDebugMessenger();
#endif
    _initialized = true;
}

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

#ifndef NDEBUG
VKAPI_ATTR vk::Bool32 VKAPI_CALL torpedoDebugMessengerCallback(
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

void tpd::Renderer::createDebugMessenger() {
    using MessageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT;
    using MessageType = vk::DebugUtilsMessageTypeFlagBitsEXT;

    auto debugInfo = vk::DebugUtilsMessengerCreateInfoEXT{};
    debugInfo.messageSeverity = MessageSeverity::eVerbose | MessageSeverity::eWarning | MessageSeverity::eError;
    debugInfo.messageType = MessageType::eGeneral | MessageType::eValidation | MessageType::ePerformance;
    debugInfo.pfnUserCallback = torpedoDebugMessengerCallback;

    if (bootstrap::createDebugUtilsMessenger(_instance, &debugInfo, &_debugMessenger) == vk::Result::eErrorExtensionNotPresent) [[unlikely]] {
        throw std::runtime_error("Renderer - Failed to set up a debug messenger: the extension is not present");
    }
}
#endif // NDEBUG

void tpd::Renderer::createInstance(std::vector<const char*>&& instanceExtensions) {
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

std::vector<const char*> tpd::Renderer::getDeviceExtensions() const {
    rendering::logExtensions("Device", "tpd::Renderer");
    return {};
}

void tpd::Renderer::engineInit(
    const vk::Device device,
    const PhysicalDeviceSelection& physicalDeviceSelection)
{
    _physicalDevice = physicalDeviceSelection.physicalDevice;
    _device = device;
    _engineInitialized = true;
}

vk::Instance tpd::Renderer::getVulkanInstance() const {
    if (!_initialized) [[unlikely]] {
        throw std::runtime_error(
            "Renderer - Fatal access: cannot get vk::Instance on an uninitialized renderer, "
            "did you forget to call tpd::Renderer::init(...)?");
    }
    return _instance;
}

void tpd::Renderer::resetEngine() noexcept {
    if (_engineInitialized) {
        _engineInitialized = false;
        _device = nullptr;
        _physicalDevice = nullptr;
    }
}

void tpd::Renderer::reset() noexcept {
    resetEngine();
    if (_initialized) {
        _initialized = false;
#ifndef NDEBUG
        bootstrap::destroyDebugUtilsMessenger(_instance, _debugMessenger);
#endif
        _instance.destroy();
    }
}

tpd::Renderer::~Renderer() noexcept {
    Renderer::reset();
}

