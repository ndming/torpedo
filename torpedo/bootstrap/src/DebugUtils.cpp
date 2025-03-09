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

std::string tpd::bootstrap::toString(const vk::PresentModeKHR presentMode) {
    using enum vk::PresentModeKHR;
    switch (presentMode) {
    case eImmediate:               return "Immediate";
    case eMailbox:                 return "Mailbox";
    case eFifo :                   return "Fifo";
    case eFifoRelaxed:             return "FifoRelaxed";
    case eSharedDemandRefresh:     return "SharedDemandRefresh";
    case eSharedContinuousRefresh: return "SharedContinuousRefresh";
    default: return "Unrecognized present mode: " + std::to_string(static_cast<int>(presentMode));
    }
}

std::string tpd::bootstrap::toString(const vk::Extent2D extent) {
    return "(" + std::to_string(extent.width) + ", " + std::to_string(extent.height) + ")";
}
