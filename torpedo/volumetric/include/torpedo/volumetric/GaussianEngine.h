#pragma once

#include <torpedo/rendering/Engine.h>
#include <torpedo/foundation/Target.h>
#include <torpedo/foundation/ShaderLayout.h>

namespace tpd {
    class GaussianEngine final : public Engine {
    public:
        void preFramePass();

        [[nodiscard]] DrawPackage draw(vk::Image image) override;

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

        static void framebufferResizeCallback(void* ptr, uint32_t width, uint32_t height);
        void onFramebufferResize(uint32_t width, uint32_t height);

        void createRenderTargets(uint32_t width, uint32_t height);
        std::pmr::vector<Target> _targets{ &_engineResourcePool };
        std::pmr::vector<vk::ImageView> _targetViews{ &_engineResourcePool };

        void createPipelineResources();
        void setTargetDescriptors() const;
        std::unique_ptr<ShaderLayout, Deleter<ShaderLayout>> _shaderLayout{};
        std::unique_ptr<ShaderInstance, Deleter<ShaderInstance>> _shaderInstance{};
        vk::Pipeline _pipeline{};

        void createComputeCommandPool();
        vk::CommandPool _computeCommandPool{};

        void createComputeCommandBuffers();
        std::pmr::vector<vk::CommandBuffer> _computeCommandBuffers{ &_engineResourcePool };

        struct ComputeSync {
            vk::Semaphore ownershipSemaphore{};
            vk::Fence computeDrawFence{};
        };
        void createComputeSyncs();
        std::pmr::vector<ComputeSync> _computeSyncs{ &_engineResourcePool };

        void recordComputeDispatchCommands(vk::CommandBuffer cmd, uint32_t currentFrame);
        void recordCopyToSwapImageCommands(vk::CommandBuffer cmd, vk::Image swapImage, uint32_t currentFrame) const;

        void cleanupRenderTargets() noexcept;
        void destroy() noexcept override;
    };
}  // namespace tpd

inline const char* tpd::GaussianEngine::getName() const noexcept {
    return "tpd::GaussianEngine";
}

inline tpd::GaussianEngine::~GaussianEngine() noexcept {
    destroy();
}
