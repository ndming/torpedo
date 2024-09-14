#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include <plog/Init.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Formatters/TxtFormatter.h>

#include <torpedo/core/DeviceBuilder.h>
#include <torpedo/core/InstanceBuilder.h>
#include <torpedo/core/PhysicalDeviceSelector.h>

#ifndef NDEBUG
#include <plog/Log.h>

VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
    const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    [[maybe_unused]] const VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    [[maybe_unused]] void* const userData
) {
    switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            PLOGV << pCallbackData->pMessage; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            PLOGI << pCallbackData->pMessage; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            PLOGW << pCallbackData->pMessage; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            PLOGE << pCallbackData->pMessage; break;
        default: break;
    }

    return vk::False;
}
#endif

static constexpr std::array DEVICE_EXTENSIONS = {
    vk::KHRSwapchainExtensionName,
};

int main() {
    // Plant a logger
    auto appender = plog::ColorConsoleAppender<plog::TxtFormatter>();
#ifdef NDEBUG
    init(plog::info, &appender);
#else
    init(plog::debug, &appender);
#endif

    // Create a Window
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    const auto window = glfwCreateWindow(1280, 768, "Hello, Tordemo!", nullptr, nullptr);

    // Create a Vulkan instance
    uint32_t glfwExtensionCount{ 0 };
    const auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    auto requiredExtensions = std::vector(glfwExtensions, glfwExtensions + glfwExtensionCount);

    auto instanceCreateFlags = vk::InstanceCreateFlags{};

#ifdef __APPLE__
    // Beginning with the 1.3.216 Vulkan SDK, the VK_KHR_PORTABILITY_subset extension is mandatory
    // for macOS with the latest MoltenVK SDK
    requiredExtensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
    instanceCreateFlags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif

    const auto debugInfo = tpd::core::createDebugInfo(debugCallback);
    const auto instance = tpd::InstanceBuilder()
        .applicationInfo(tpd::core::createApplicationInfo("tordemo", vk::makeApiVersion(0, 1, 3, 0)))
#ifndef NDEBUG
        .debugInfo(debugInfo)
        .layers({ "VK_LAYER_KHRONOS_validation" })
#endif
        .extensions(requiredExtensions)
        .build(instanceCreateFlags);

    // Set up a debug messenger
    auto debugMessenger = vk::DebugUtilsMessengerEXT{};
    if (tpd::core::createDebugUtilsMessenger(instance, &debugInfo, &debugMessenger) == vk::Result::eErrorExtensionNotPresent) {
        throw std::runtime_error("Failed to set up a debug messenger!");
    }

    // Create a surface
    auto surface = vk::SurfaceKHR{};
    if (glfwCreateWindowSurface(instance, window, nullptr, reinterpret_cast<VkSurfaceKHR*>(&surface)) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create a window surface!");
    }

    // Pick a physical device
    const auto selector = tpd::PhysicalDeviceSelector()
        .deviceExtensions(DEVICE_EXTENSIONS.data(), DEVICE_EXTENSIONS.size())
        .requestGraphicsQueueFamily()
        .requestPresentQueueFamily(surface)
        .select(instance);
    const auto physicalDevice = selector.getPhysicalDevice();
    const auto graphicsFamily = selector.getGraphicsQueueFamily();
    const auto presentFamily = selector.getPresentQueueFamily();

    const auto device = tpd::DeviceBuilder()
        .deviceExtensions(DEVICE_EXTENSIONS.data(), DEVICE_EXTENSIONS.size())
        .queueFamilyIndices({ graphicsFamily, presentFamily })
        .build(physicalDevice);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    device.destroy();
    instance.destroySurfaceKHR(surface);
    tpd::core::destroyDebugUtilsMessenger(instance, debugMessenger);
    instance.destroy();

    glfwDestroyWindow(window);
    glfwTerminate();
}
