#pragma once

#include <vulkan/vulkan.hpp>

namespace tpd::bootstrap {
    [[nodiscard]] vk::Result createDebugUtilsMessenger(
            vk::Instance instance,
            const vk::DebugUtilsMessengerCreateInfoEXT* pCreateInfo,
            vk::DebugUtilsMessengerEXT* pDebugMessenger,
            const vk::AllocationCallbacks* pAllocator = nullptr);

    void destroyDebugUtilsMessenger(
        vk::Instance instance,
        vk::DebugUtilsMessengerEXT debugMessenger,
        const vk::AllocationCallbacks* pAllocator = nullptr);

    void setVulkanObjectName(
        auto handle, vk::ObjectType type, std::string_view name,
        vk::Instance instance, vk::Device device);

    [[nodiscard]] std::string toString(vk::PresentModeKHR presentMode);
    [[nodiscard]] std::string toString(vk::Extent2D extent);
}

// =====================================================================================================================
// TEMPLATE FUNCTION DEFINITIONS
// =====================================================================================================================

void tpd::bootstrap::setVulkanObjectName(
    const auto handle,
    const vk::ObjectType type,
    const std::string_view name,
    const vk::Instance instance,
    const vk::Device device)
{
    constexpr auto setObjNameFn = "vkSetDebugUtilsObjectNameEXT";
    if (const auto func = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(instance, setObjNameFn));
        func != nullptr) {
        const auto nameInfo = VkDebugUtilsObjectNameInfoEXT{
            .sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .objectType   = static_cast<VkObjectType>(type),
            .objectHandle = reinterpret_cast<uint64_t>(handle),
            .pObjectName  = name.data()
        };
        func(device, &nameInfo);
    }
}
