#pragma once

#include <vulkan/vulkan.hpp>

#include <set>

namespace tpd {
    class DeviceBuilder {
    public:
        DeviceBuilder& deviceExtensions(const char* const* extensions, std::size_t count);
        DeviceBuilder& deviceExtensions(const std::vector<const char*>& extensions);
        DeviceBuilder& deviceExtensions(std::vector<const char*>&& extensions) noexcept;

        DeviceBuilder& deviceFeatures(const vk::PhysicalDeviceFeatures2& features);

        DeviceBuilder& queueFamilyIndices(std::initializer_list<uint32_t> families);
        DeviceBuilder& queueFamilyIndices(const std::set<uint32_t>& families);
        DeviceBuilder& queueFamilyIndices(std::set<uint32_t>&& families) noexcept;

        [[nodiscard]] vk::Device build(vk::PhysicalDevice physicalDevice) const;

    private:
        std::vector<const char*> _extensions{};

        vk::PhysicalDeviceFeatures2 _features{};

        std::vector<const char*> _layers{};

        std::set<uint32_t> _queueFamilies{};
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::DeviceBuilder& tpd::DeviceBuilder::deviceExtensions(const char* const* extensions, const std::size_t count) {
    return this->deviceExtensions(std::vector(extensions, extensions + count));
}

inline tpd::DeviceBuilder& tpd::DeviceBuilder::deviceExtensions(const std::vector<const char*>& extensions) {
    _extensions = extensions;
    return *this;
}

inline tpd::DeviceBuilder& tpd::DeviceBuilder::deviceExtensions(std::vector<const char*>&& extensions) noexcept {
    _extensions = std::move(extensions);
    return *this;
}

inline tpd::DeviceBuilder& tpd::DeviceBuilder::deviceFeatures(const vk::PhysicalDeviceFeatures2& features) {
    _features = features;
    return *this;
}

inline tpd::DeviceBuilder& tpd::DeviceBuilder::queueFamilyIndices(const std::initializer_list<uint32_t> families) {
    return queueFamilyIndices(std::set(families.begin(), families.end()));
}

inline tpd::DeviceBuilder& tpd::DeviceBuilder::queueFamilyIndices(const std::set<uint32_t>& families) {
    _queueFamilies = families;
    return *this;
}

inline tpd::DeviceBuilder&tpd::DeviceBuilder::queueFamilyIndices(std::set<uint32_t>&& families) noexcept {
    _queueFamilies = std::move(families);
    return *this;
}
