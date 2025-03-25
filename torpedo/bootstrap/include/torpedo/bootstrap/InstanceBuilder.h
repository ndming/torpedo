#pragma once

#include <vulkan/vulkan.hpp>

namespace tpd {
    class InstanceBuilder {
    public:
        InstanceBuilder& applicationVersion(uint32_t major, uint32_t minor, uint32_t patch) noexcept;
        InstanceBuilder& apiVersion(uint32_t major, uint32_t minor, uint32_t patch) noexcept;

        InstanceBuilder& debugInfoMessageSeverityFlags(vk::DebugUtilsMessageSeverityFlagsEXT flags) noexcept;
        InstanceBuilder& debugInfoMessageTypeFlags(vk::DebugUtilsMessageTypeFlagsEXT flags) noexcept;
        InstanceBuilder& debugInfoCallback(vk::PFN_DebugUtilsMessengerCallbackEXT callback, void* pUserData = nullptr) noexcept;

        InstanceBuilder& extensions(const std::vector<const char*>& extensions);
        InstanceBuilder& extensions(std::vector<const char*>&& extensions) noexcept;

        [[nodiscard]] vk::Instance build(vk::InstanceCreateFlags flags = {}, const std::vector<const char*>& layers = {});

    private:
        vk::ApplicationInfo _applicationInfo{
            "torpedo",
            vk::makeApiVersion(0, 1, 0, 0),
            "torpedo",
            vk::makeApiVersion(0, 1, 0, 0),
            vk::makeApiVersion(0, 1, 3, 0),
        };

        using MessageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT;
        using MessageType = vk::DebugUtilsMessageTypeFlagBitsEXT;
        vk::DebugUtilsMessengerCreateInfoEXT _debugInfo{
            {},
            MessageSeverity::eVerbose | MessageSeverity::eWarning | MessageSeverity::eError,
            MessageType::eGeneral | MessageType::eValidation | MessageType::ePerformance,
            vk::PFN_DebugUtilsMessengerCallbackEXT{ nullptr },
            nullptr
        };

        std::vector<const char*> _extensions{};

        static bool allLayersAvailable(const std::vector<const char*>& layers);
    };
}

// INLINE FUNCTION DEFINITIONS
// ---------------------------

inline tpd::InstanceBuilder& tpd::InstanceBuilder::applicationVersion(const uint32_t major, const uint32_t minor, const uint32_t patch) noexcept {
    _applicationInfo.applicationVersion = vk::makeApiVersion(0u, major, minor, patch);
    return *this;
}

inline tpd::InstanceBuilder& tpd::InstanceBuilder::apiVersion(const uint32_t major, const uint32_t minor, const uint32_t patch) noexcept {
    _applicationInfo.apiVersion = vk::makeApiVersion(0u, major, minor, patch);
    return *this;
}

inline tpd::InstanceBuilder& tpd::InstanceBuilder::debugInfoMessageSeverityFlags(const vk::DebugUtilsMessageSeverityFlagsEXT flags) noexcept {
    _debugInfo.messageSeverity = flags;
    return *this;
}

inline tpd::InstanceBuilder& tpd::InstanceBuilder::debugInfoMessageTypeFlags(const vk::DebugUtilsMessageTypeFlagsEXT flags) noexcept {
    _debugInfo.messageType = flags;
    return *this;
}

inline tpd::InstanceBuilder&tpd::InstanceBuilder::debugInfoCallback(vk::PFN_DebugUtilsMessengerCallbackEXT callback, void* pUserData) noexcept {
    _debugInfo.pfnUserCallback = callback;
    _debugInfo.pUserData = pUserData;
    return *this;
}

inline tpd::InstanceBuilder&tpd::InstanceBuilder::extensions(const std::vector<const char*>& extensions) {
    _extensions = extensions;
    return *this;
}

inline tpd::InstanceBuilder&tpd::InstanceBuilder::extensions(std::vector<const char*>&& extensions) noexcept {
    _extensions = std::move(extensions);
    return *this;
}
