#pragma once

#include "torpedo/renderer/StandardRenderer.h"

namespace tpd {
    class ForwardRenderer final : public StandardRenderer {
    public:
        explicit ForwardRenderer(void* window) : StandardRenderer{ static_cast<GLFWwindow*>(window) } {}

        [[nodiscard]] vk::GraphicsPipelineCreateInfo getGraphicsPipelineInfo(float minSampleShading) const override;

        ForwardRenderer(const ForwardRenderer&) = delete;
        ForwardRenderer& operator=(const ForwardRenderer&) = delete;

        ~ForwardRenderer() override;

    private:
        void createFramebufferResources() override;
        void destroyFramebufferResources() const noexcept override;

        // Framebuffer color resources
        vk::Image _framebufferColorImage{};
        vk::ImageView _framebufferColorImageView{};
        VmaAllocation _framebufferColorImageAllocation{ nullptr };
        void createFramebufferColorResources();

        // Framebuffer depth resources
        vk::Image _framebufferDepthImage{};
        vk::ImageView _framebufferDepthImageView{};
        VmaAllocation _framebufferDepthImageAllocation{ nullptr };
        vk::Format _framebufferDepthImageFormat{ vk::Format::eUndefined };
        void createFramebufferDepthResources();
        [[nodiscard]] vk::Format findFramebufferDepthImageFormat(
            const std::vector<vk::Format>& candidates,
            vk::ImageTiling tiling,
            vk::FormatFeatureFlags features) const;

        void createRenderPass() override;
        void createFramebuffers() override;

        [[nodiscard]] std::vector<vk::ClearValue> getClearValues() const override;
        void onDraw(const View& view, vk::CommandBuffer buffer) const override;
    };
}
