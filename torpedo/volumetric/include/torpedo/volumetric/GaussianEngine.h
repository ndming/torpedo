#pragma once

#include <torpedo/rendering/Engine.h>
#include <torpedo/foundation/Target.h>

namespace tpd {
    class GaussianEngine final : public Engine {
    public:
        [[nodiscard]] DrawPackage draw(vk::Image image) const override;

        [[nodiscard]] const char* getName() const noexcept override;

    private:
        [[nodiscard]] PhysicalDeviceSelection pickPhysicalDevice(
            const std::vector<const char*>& deviceExtensions,
            vk::Instance instance, vk::SurfaceKHR surface) const override;

        [[nodiscard]] vk::Device createDevice(
            const std::vector<const char*>& deviceExtensions,
            std::initializer_list<uint32_t> queueFamilyIndices) const override;
        
        static vk::PhysicalDeviceVulkan13Features getVulkan13Features();

        void onInitialized() override;

        std::pmr::vector<Target> _targets{};

        void destroy() noexcept override;
    };
}  // namespace tpd

inline const char* tpd::GaussianEngine::getName() const noexcept {
    return "tpd::GaussianEngine";
}