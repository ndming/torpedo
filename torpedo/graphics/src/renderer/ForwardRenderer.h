#pragma once

#include "renderer/StandardRenderer.h"

namespace tpd::renderer {
    class ForwardRenderer final : public StandardRenderer {
    public:
        explicit ForwardRenderer(GLFWwindow* window) : StandardRenderer{ window } {}

        [[nodiscard]] vk::GraphicsPipelineCreateInfo getGraphicsPipelineInfo() const override;

        ForwardRenderer(const ForwardRenderer&) = delete;
        ForwardRenderer& operator=(const ForwardRenderer&) = delete;

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
        void onDrawBegin(const View& view) const override;
        void onDraw(const View& view, vk::CommandBuffer buffer) const override;
    };
}
