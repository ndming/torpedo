#include "torpedo/bootstrap/DebugUtils.h"

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
