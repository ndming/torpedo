#pragma once

#include <torpedo/rendering/Engine.h>

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

        void onInitialized() override;
    };
}  // namespace tpd

inline const char* tpd::GaussianEngine::getName() const noexcept {
    return "tpd::GaussianEngine";
}