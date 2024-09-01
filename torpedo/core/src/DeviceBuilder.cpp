#include "torpedo/core/DeviceBuilder.h"

#include <ranges>

VkDevice tpd::DeviceBuilder::build(const VkPhysicalDevice& physicalDevice) const {
    return build(static_cast<vk::PhysicalDevice>(physicalDevice));
}

vk::Device tpd::DeviceBuilder::build(const vk::PhysicalDevice& physicalDevice) const {
    using namespace std::views;
    constexpr auto priority = 1.0f;
    const auto toQueueCreateInfo = [&priority](const auto& family) { return vk::DeviceQueueCreateInfo{ {}, family, 1, &priority }; };
    const auto queueCreateInfos = _queueFamilies | transform(toQueueCreateInfo) | std::ranges::to<std::vector>();

    auto deviceCreateInfo = vk::DeviceCreateInfo{};
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pEnabledFeatures = nullptr;  // we're using vk::PhysicalDeviceFeatures2 specified in the pNext ptr
    deviceCreateInfo.pNext = &_features;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(_extensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = _extensions.data();
    // Up-to-date implementations of Vulkan will ignore these layer-related fields
    deviceCreateInfo.enabledLayerCount = 0;

    return physicalDevice.createDevice(deviceCreateInfo);
}
