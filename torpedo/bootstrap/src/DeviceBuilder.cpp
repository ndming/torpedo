#include "torpedo/bootstrap/DeviceBuilder.h"

#include <ranges>

tpd::DeviceBuilder& tpd::DeviceBuilder::deviceFeatures(vk::PhysicalDeviceFeatures2* const features) noexcept {
    _features = features;
    return *this;
}

tpd::DeviceBuilder& tpd::DeviceBuilder::queueFamilyIndices(const std::initializer_list<uint32_t> families) {
    if (families.size() > MAX_UNIQUE_FAMILIES) [[unlikely]] {
        throw std::runtime_error(
            std::format("DeviceBuilder - More than {} queue family indices are being processed.", MAX_UNIQUE_FAMILIES));
    }

    // Convert uint32_t to vk::DeviceQueueCreateInfo and store in a temp array
    constexpr auto priority = 1.0f;
    const auto toQueueCreateInfo = [&priority](const auto idx) { return vk::DeviceQueueCreateInfo{ {}, idx, 1, &priority }; };
    const auto queueInfos = families | std::views::transform(toQueueCreateInfo);

    auto temp = std::array<vk::DeviceQueueCreateInfo, MAX_UNIQUE_FAMILIES>{};
    const auto [in, out] = std::ranges::copy(queueInfos.begin(), queueInfos.end(), temp.begin());

    // Sort to filter unique elements, this may seem verbose, but we avoid allocating on the heap
    const auto less = [](const auto& lhs, const auto& rhs) { return lhs.queueFamilyIndex <  rhs.queueFamilyIndex; };
    std::ranges::sort(temp.begin(), out, less);
    const auto comp = [](const auto& lhs, const auto& rhs) { return lhs.queueFamilyIndex == rhs.queueFamilyIndex; };
    const auto [_, last] = std::ranges::unique_copy(temp.begin(), out, _queueInfos.begin(), comp);
    _queueInfoCount = std::distance(_queueInfos.begin(), last);

    return *this;
}

vk::Device tpd::DeviceBuilder::build(
    const vk::PhysicalDevice physicalDevice,
    const vk::ArrayProxy<const char*>& extensions) const
{
    const auto deviceCreateInfo = vk::DeviceCreateInfo{}
        .setPQueueCreateInfos(_queueInfos.data())
        .setQueueCreateInfoCount(_queueInfoCount)
        .setEnabledExtensionCount(extensions.size())
        .setPpEnabledExtensionNames(extensions.data())
        .setPEnabledFeatures(nullptr)  // we're using vk::PhysicalDeviceFeatures2 ...
        .setPNext(_features)           // ... specified in the pNext ptr
        .setEnabledLayerCount(0);      // obsolete in up-to-date Vulkan
    return physicalDevice.createDevice(deviceCreateInfo);
}
