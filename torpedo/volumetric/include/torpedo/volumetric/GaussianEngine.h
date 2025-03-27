#pragma once

#include <torpedo/rendering/Engine.h>
#include <torpedo/foundation/Target.h>
#include <torpedo/foundation/ShaderLayout.h>

namespace tpd {
    class GaussianEngine final : public Engine {
    public:
        [[nodiscard]] DrawPackage draw(vk::Image image) const override;

        [[nodiscard]] const char* getName() const noexcept override;

        ~GaussianEngine() noexcept override;

    private:
        [[nodiscard]] PhysicalDeviceSelection pickPhysicalDevice(
            const std::vector<const char*>& deviceExtensions,
            vk::Instance instance, vk::SurfaceKHR surface) const override;

        [[nodiscard]] vk::Device createDevice(
            const std::vector<const char*>& deviceExtensions,
            std::initializer_list<uint32_t> queueFamilyIndices) const override;
        
        static vk::PhysicalDeviceVulkan13Features getVulkan13Features();

        void onInitialized() override;

        void createRenderTargets();
        std::pmr::vector<Target> _targets{ &_engineResourcePool };
        std::pmr::vector<vk::ImageView> _targetViews{ &_engineResourcePool };

        void createPipelineResources();
        std::unique_ptr<ShaderLayout, Deleter<ShaderLayout>> _shaderLayout{};
        std::unique_ptr<ShaderInstance, Deleter<ShaderInstance>> _shaderInstance{};
        vk::Pipeline _pipeline{};

        void destroy() noexcept override;
    };
}  // namespace tpd

inline const char* tpd::GaussianEngine::getName() const noexcept {
    return "tpd::GaussianEngine";
}

inline tpd::GaussianEngine::~GaussianEngine() noexcept {
    destroy();
}
