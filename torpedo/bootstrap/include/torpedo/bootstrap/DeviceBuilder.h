#pragma once

#include <vulkan/vulkan.hpp>

namespace tpd {
    class DeviceBuilder {
    public:
        /**
         * Specifies which Vulkan features to enable when creating the device. The passed in pointer must remain valid
         * before `DeviceBuilder::build` returns.
         *
         * @param features a pointer to `vk::PhysicalDeviceFeatures2` which may have chaining structures through `pNext`.
         * @return `this` object for chaining calls.
         */
        DeviceBuilder& deviceFeatures(vk::PhysicalDeviceFeatures2* features) noexcept;

        /**
         * Specifies queue family indices involved in the creation of the device. The indices may be provided in
         * any other and can be duplicated. The builder makes sure to derive a unique set of indices.
         *
         * @param families a list of queue family indices.
         * @return `this` object for chaining calls.
         */
        DeviceBuilder& queueFamilyIndices(std::initializer_list<uint32_t> families);

        /**
         * Creates a Vulkan logical device from the provided physical device and optionally a list of device extensions.
         */
        [[nodiscard]] vk::Device build(
            vk::PhysicalDevice physicalDevice,
            const vk::ArrayProxy<const char*>& extensions = {}) const;

    private:
        vk::PhysicalDeviceFeatures2* _features{ nullptr };

        static constexpr auto MAX_UNIQUE_FAMILIES = 8;
        std::array<vk::DeviceQueueCreateInfo, MAX_UNIQUE_FAMILIES> _queueInfos{};
        uint32_t _queueInfoCount{ 0 };
    };
}  // namespace tpd
