#include "torpedo/bootstrap/InstanceBuilder.h"

#include <ranges>
#include <unordered_set>

vk::Instance tpd::InstanceBuilder::build(const vk::InstanceCreateFlags flags, const std::vector<const char*>& layers) {
    if (!allLayersAvailable(layers)) [[unlikely]] {
        throw std::runtime_error("InstanceBuilder - Not all requested layers are available, consider updating your drivers");
    }

    auto createInfo = vk::InstanceCreateInfo{};
    createInfo.flags = flags;
    createInfo.pApplicationInfo = &_applicationInfo;

    if (_debugInfo.pfnUserCallback) {
        _extensions.push_back(vk::EXTDebugUtilsExtensionName);
        createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
        createInfo.ppEnabledLayerNames = layers.data();
        createInfo.pNext = &_debugInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    createInfo.enabledExtensionCount = static_cast<uint32_t>(_extensions.size());
    createInfo.ppEnabledExtensionNames = _extensions.data();

    return createInstance(createInfo);
}

bool tpd::InstanceBuilder::allLayersAvailable(const std::vector<const char*>& layers) {
    const auto properties = vk::enumerateInstanceLayerProperties();
    const auto toName = [](const auto& property) { return property.layerName.data(); };
    const auto supportedLayers = properties | std::views::transform(toName) | std::ranges::to<std::unordered_set<std::string>>();

    const auto supported = [&supportedLayers](const auto& it) { return supportedLayers.contains(it); };
    return std::ranges::all_of(layers, std::identity{}, supported);
}

vk::Result tpd::bootstrap::createDebugUtilsMessenger(
    const vk::Instance instance,
    const vk::DebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    vk::DebugUtilsMessengerEXT* pDebugMessenger,
    const vk::AllocationCallbacks* pAllocator)
{
    constexpr auto createDebugFn = "vkCreateDebugUtilsMessengerEXT";
    if (const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, createDebugFn));
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
    const vk::Instance instance,
    const vk::DebugUtilsMessengerEXT debugMessenger,
    const vk::AllocationCallbacks *pAllocator)
{
    constexpr auto destroyDebugFn = "vkDestroyDebugUtilsMessengerEXT";
    if (const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, destroyDebugFn));
        func != nullptr) {
        func(instance, debugMessenger, reinterpret_cast<const VkAllocationCallbacks*>(pAllocator));
    }
}
