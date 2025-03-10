#pragma once

#include "torpedo/rendering/Engine.h"

namespace tpd {
    class ForwardEngine final : public Engine {
    public:
        ForwardEngine() = default;

        ForwardEngine(const ForwardEngine&) = delete;
        ForwardEngine& operator=(const ForwardEngine&) = delete;

        [[nodiscard]] vk::CommandBuffer draw(vk::Image image) const override;

        [[nodiscard]] const char* getName() const noexcept override;

        void destroy() noexcept override;
        ~ForwardEngine() noexcept override;

    private:
        [[nodiscard]] std::vector<const char*> getDeviceExtensions() const override;

        [[nodiscard]] PhysicalDeviceSelection pickPhysicalDevice(
            const std::vector<const char*>& deviceExtensions,
            vk::Instance instance, vk::SurfaceKHR surface) const override;

        [[nodiscard]] vk::Device createDevice(
            const std::vector<const char*>& deviceExtensions,
            std::initializer_list<uint32_t> queueFamilyIndices) const override;

        static vk::PhysicalDeviceVulkan13Features getVulkan13Features();
        static vk::PhysicalDeviceVulkan12Features getVulkan12Features();
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline const char* tpd::ForwardEngine::getName() const noexcept {
    return "tpd::ForwardEngine";
}
