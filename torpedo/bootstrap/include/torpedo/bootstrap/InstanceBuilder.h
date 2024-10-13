#pragma once

#include <vulkan/vulkan.hpp>

namespace tpd {
    class InstanceBuilder {
    public:
        InstanceBuilder& applicationInfo(const vk::ApplicationInfo& info);
        InstanceBuilder& applicationInfo(const VkApplicationInfo& info);

        InstanceBuilder& debugInfo(const vk::DebugUtilsMessengerCreateInfoEXT& info);

        InstanceBuilder& extensions(const char* const* extensions, std::size_t count);
        InstanceBuilder& extensions(const std::vector<const char*>& extensions);
        InstanceBuilder& extensions(std::vector<const char*>&& extensions) noexcept;

        InstanceBuilder& layers(const char* const* layers, std::size_t count);
        InstanceBuilder& layers(const std::vector<const char*>& layers);
        InstanceBuilder& layers(std::vector<const char*>&& layers) noexcept;

        [[nodiscard]] vk::Instance build(vk::InstanceCreateFlags flags = {});

    private:
        vk::ApplicationInfo _applicationInfo{};

        vk::DebugUtilsMessengerCreateInfoEXT _debugInfo{};

        std::vector<const char*> _extensions{};

        std::vector<const char*> _layers{};
        [[nodiscard]] bool allLayersAvailable() const;
    };

    namespace bootstrap {
        vk::ApplicationInfo createApplicationInfo(std::string_view name, uint32_t apiVersion);
        vk::DebugUtilsMessengerCreateInfoEXT createDebugInfo(PFN_vkDebugUtilsMessengerCallbackEXT callback, void* pUserData = nullptr);

        vk::Result createDebugUtilsMessenger(
            const vk::Instance& instance,
            const vk::DebugUtilsMessengerCreateInfoEXT* pCreateInfo,
            vk::DebugUtilsMessengerEXT* pDebugMessenger,
            const vk::AllocationCallbacks* pAllocator = nullptr);

        void destroyDebugUtilsMessenger(
            const vk::Instance& instance,
            vk::DebugUtilsMessengerEXT debugMessenger,
            const vk::AllocationCallbacks* pAllocator = nullptr);
    }
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::InstanceBuilder&tpd::InstanceBuilder::applicationInfo(const vk::ApplicationInfo& info) {
    _applicationInfo = info;
    return *this;
}

inline tpd::InstanceBuilder&tpd::InstanceBuilder::applicationInfo(const VkApplicationInfo& info) {
    _applicationInfo = info;
    return *this;
}

inline tpd::InstanceBuilder&tpd::InstanceBuilder::debugInfo(const vk::DebugUtilsMessengerCreateInfoEXT& info) {
    _debugInfo = info;
    return *this;
}

inline tpd::InstanceBuilder&tpd::InstanceBuilder::extensions(const char* const* extensions, const std::size_t count) {
    return this->extensions(std::vector(extensions, extensions + count));
}

inline tpd::InstanceBuilder&tpd::InstanceBuilder::extensions(const std::vector<const char*>& extensions) {
    _extensions = extensions;
    return *this;
}

inline tpd::InstanceBuilder&tpd::InstanceBuilder::extensions(std::vector<const char*>&& extensions) noexcept {
    _extensions = std::move(extensions);
    return *this;
}

inline tpd::InstanceBuilder&tpd::InstanceBuilder::layers(const char* const* layers, const std::size_t count) {
    return this->layers(std::vector(layers, layers + count));
}

inline tpd::InstanceBuilder&tpd::InstanceBuilder::layers(const std::vector<const char*>& layers) {
    _layers = layers;
    return *this;
}

inline tpd::InstanceBuilder&tpd::InstanceBuilder::layers(std::vector<const char*>&& layers) noexcept {
    _layers = std::move(layers);
    return *this;
}

inline vk::ApplicationInfo tpd::bootstrap::createApplicationInfo(const std::string_view name, const uint32_t apiVersion) {
    return vk::ApplicationInfo{ name.data(), vk::makeApiVersion(0, 1, 0, 0), "None", apiVersion, apiVersion };
}
