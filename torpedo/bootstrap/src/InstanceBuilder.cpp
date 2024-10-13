#include "torpedo/bootstrap/InstanceBuilder.h"

#include <ranges>
#include <unordered_set>

vk::Instance tpd::InstanceBuilder::build(const vk::InstanceCreateFlags flags) {
    // Check if all required layers are available, throw if any of them is not supported
    if (!allLayersAvailable()) {
        throw std::runtime_error("InstanceBuilder - Not all requested layers are available, consider update your drivers");
    }

    auto createInfo = vk::InstanceCreateInfo{};
    createInfo.flags = flags;
    createInfo.pApplicationInfo = &_applicationInfo;

    if (_debugInfo.pfnUserCallback) {
        _extensions.push_back(vk::EXTDebugUtilsExtensionName);
        createInfo.enabledLayerCount = static_cast<uint32_t>(_layers.size());
        createInfo.ppEnabledLayerNames = _layers.data();
        createInfo.pNext = &_debugInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    createInfo.enabledExtensionCount = static_cast<uint32_t>(_extensions.size());
    createInfo.ppEnabledExtensionNames = _extensions.data();

    return createInstance(createInfo);
}

bool tpd::InstanceBuilder::allLayersAvailable() const {
    using namespace std::ranges;
    const auto properties = vk::enumerateInstanceLayerProperties();

    const auto toName = [](const auto& property) { return property.layerName.data(); };
    const auto supportedLayers = properties | views::transform(toName) | to<std::unordered_set<std::string>>();

    const auto supported = [&supportedLayers](const auto& it) { return supportedLayers.contains(it); };
    return all_of(_layers, std::identity{}, supported);
}

vk::DebugUtilsMessengerCreateInfoEXT tpd::bootstrap::createDebugInfo(PFN_vkDebugUtilsMessengerCallbackEXT callback, void* pUserData) {
    using MessageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT;
    using MessageType = vk::DebugUtilsMessageTypeFlagBitsEXT;

    auto debugInfo = vk::DebugUtilsMessengerCreateInfoEXT{};
    debugInfo.messageSeverity = MessageSeverity::eVerbose | MessageSeverity::eWarning | MessageSeverity::eError;
    debugInfo.messageType = MessageType::eGeneral | MessageType::eValidation | MessageType::ePerformance;
    debugInfo.pfnUserCallback = callback;
    debugInfo.pUserData = pUserData;

    return debugInfo;
}

vk::Result tpd::bootstrap::createDebugUtilsMessenger(
    const vk::Instance& instance,
    const vk::DebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    vk::DebugUtilsMessengerEXT* pDebugMessenger,
    const vk::AllocationCallbacks* pAllocator)
{
    static constexpr auto createFunc = "vkCreateDebugUtilsMessengerEXT";
    if (const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, createFunc));
        func != nullptr) {
        return static_cast<vk::Result>(
            func(
                instance, reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(pCreateInfo),
                reinterpret_cast<const VkAllocationCallbacks*>(pAllocator),
                reinterpret_cast<VkDebugUtilsMessengerEXT*>(pDebugMessenger)
            )
        );
    }
    return vk::Result::eErrorExtensionNotPresent;
}

void tpd::bootstrap::destroyDebugUtilsMessenger(
    const vk::Instance& instance,
    const vk::DebugUtilsMessengerEXT debugMessenger,
    const vk::AllocationCallbacks *pAllocator)
{
    static constexpr auto destroyFunc = "vkDestroyDebugUtilsMessengerEXT";
    if (const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, destroyFunc));
        func != nullptr) {
        func(instance, debugMessenger, reinterpret_cast<const VkAllocationCallbacks*>(pAllocator));
    }
}
