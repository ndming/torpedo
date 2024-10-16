#include "renderer/raster/ForwardRenderer.h"

#include <ranges>

void tpd::renderer::ForwardRenderer::createFramebufferResources() {
    createFramebufferColorResources();
    createFramebufferDepthResources();
}

void tpd::renderer::ForwardRenderer::destroyFramebufferResources() const noexcept {
    _device.destroyImageView(_framebufferDepthImageView);
    _device.destroyImageView(_framebufferColorImageView);
    _allocator->freeImage(_framebufferDepthImage, _framebufferDepthImageAllocation);
    _allocator->freeImage(_framebufferColorImage, _framebufferColorImageAllocation);
}

void tpd::renderer::ForwardRenderer::createFramebufferColorResources() {
    using Usage = vk::ImageUsageFlagBits;
    constexpr auto mipLevels = 1;

    auto imageCreateInfo = vk::ImageCreateInfo{};
    imageCreateInfo.imageType = vk::ImageType::e2D;
    imageCreateInfo.format = _swapChainImageFormat;
    imageCreateInfo.extent.width = _swapChainImageExtent.width;
    imageCreateInfo.extent.height = _swapChainImageExtent.height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = mipLevels;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = _msaaSampleCount;
    imageCreateInfo.tiling = vk::ImageTiling::eOptimal;
    imageCreateInfo.usage = Usage::eTransientAttachment | Usage::eColorAttachment;
    imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    imageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;

    _framebufferColorImage = _allocator->allocateDedicatedImage(imageCreateInfo, &_framebufferColorImageAllocation);

    constexpr auto aspectFlags = vk::ImageAspectFlagBits::eColor;
    auto subresourceRange = vk::ImageSubresourceRange{};
    subresourceRange.aspectMask = aspectFlags;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = mipLevels;
    subresourceRange.layerCount = 1;

    auto imageViewCreateInfo = vk::ImageViewCreateInfo{};
    imageViewCreateInfo.image = _framebufferColorImage;
    imageViewCreateInfo.format = _swapChainImageFormat;
    imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
    imageViewCreateInfo.subresourceRange = subresourceRange;

    _framebufferColorImageView = _device.createImageView(imageViewCreateInfo);
}

void tpd::renderer::ForwardRenderer::createFramebufferDepthResources() {
    const auto formatCandidates = std::vector{
        vk::Format::eD32Sfloat,        // 32-bit float for depth
        vk::Format::eD32SfloatS8Uint,  // 32-bit signed float for depth and 8-bit stencil component
        vk::Format::eD24UnormS8Uint,   // 24-bit float for depth and 8-bit stencil component
    };

    // Choose the desired depth format
    using Tiling = vk::ImageTiling;
    using Feature = vk::FormatFeatureFlagBits;
    _framebufferDepthImageFormat = findFramebufferDepthImageFormat(formatCandidates, Tiling::eOptimal, Feature::eDepthStencilAttachment);

    constexpr auto mipLevels = 1;
    constexpr auto aspectFlags = vk::ImageAspectFlagBits::eDepth;

    auto imageCreateInfo = vk::ImageCreateInfo{};
    imageCreateInfo.imageType = vk::ImageType::e2D;
    imageCreateInfo.format = _framebufferDepthImageFormat;
    imageCreateInfo.extent.width = _swapChainImageExtent.width;
    imageCreateInfo.extent.height = _swapChainImageExtent.height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = mipLevels;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = _msaaSampleCount;
    imageCreateInfo.tiling = vk::ImageTiling::eOptimal;
    imageCreateInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
    imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    imageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;

    _framebufferDepthImage = _allocator->allocateDedicatedImage(imageCreateInfo, &_framebufferDepthImageAllocation);

    auto subresourceRange = vk::ImageSubresourceRange{};
    subresourceRange.aspectMask = aspectFlags;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = mipLevels;
    subresourceRange.layerCount = 1;

    auto imageViewCreateInfo = vk::ImageViewCreateInfo{};
    imageViewCreateInfo.image = _framebufferDepthImage;
    imageViewCreateInfo.format = _framebufferDepthImageFormat;
    imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
    imageViewCreateInfo.subresourceRange = subresourceRange;

    _framebufferDepthImageView = _device.createImageView(imageViewCreateInfo);
}

vk::Format tpd::renderer::ForwardRenderer::findFramebufferDepthImageFormat(
    const std::vector<vk::Format>& candidates,
    const vk::ImageTiling tiling,
    const vk::FormatFeatureFlags features) const
{
    // The support of a format depends on the tiling mode and usage, so we must also include these in the check
    for (const auto format : candidates) {
        const auto props = _physicalDevice.getFormatProperties(format);
        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("ForwardRenderer: failed to find a supported framebuffer depth image format");
}

void tpd::renderer::ForwardRenderer::createRenderPass() {
    const auto colorAttachment = vk::AttachmentDescription{
        {}, _swapChainImageFormat,
        _msaaSampleCount,
        vk::AttachmentLoadOp::eClear,     // clear the framebuffer to black before drawing a new frame
        vk::AttachmentStoreOp::eStore,    // rendered contents will be stored in memory to be resolved later
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal
    };
    const auto depthAttachment = vk::AttachmentDescription{
        {}, _framebufferDepthImageFormat,
        _msaaSampleCount,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eDontCare,  // we're not using depth values after drawing has finished
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal
    };
    const auto colorAttachmentResolve = vk::AttachmentDescription{
        {}, _swapChainImageFormat,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eStore,    // resolved contents will be stored in memory to be presented later
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::ePresentSrcKHR   // we want the image to be ready for presentation after rendering
    };

    // We should keep the order we specify these attachments in mind because later on we will have to specify
    // clear values for them in the same order. Also, the attachment reference objects reference attachments using the
    // indices of this array
    const auto attachments = std::array{ colorAttachment, depthAttachment, colorAttachmentResolve };

    static constexpr auto colorAttachmentRef = vk::AttachmentReference{ 0, vk::ImageLayout::eColorAttachmentOptimal };
    static constexpr auto depthAttachmentRef = vk::AttachmentReference{ 1, vk::ImageLayout::eDepthStencilAttachmentOptimal };
    static constexpr auto colorAttachmentResolveRef = vk::AttachmentReference{ 2, vk::ImageLayout::eColorAttachmentOptimal };

    // Define a subpass for our rendering
    auto subpass = vk::SubpassDescription{};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    // The index of the attachment in this array is directly referenced from the fragment shader with the
    // "layout(location = <index>) out" directive
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    // Unlike color attachments, a subpass can only use a single depth (+stencil) attachment
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    // This is enough to let the render pass define a multisample resolve operation
    subpass.pResolveAttachments = &colorAttachmentResolveRef;

    // Subpasses in a render pass automatically take care of image layout transitions. These transitions are controlled
    // by subpass dependencies, which specify memory and execution dependencies between subpasses. We have only a single
    // subpass right now, but the operations right before and right after this subpass also count as implicit "subpasses"
    // There are two built-in dependencies that take care of the transition at the start of the render pass and at the
    // end of the render pass, but the former does not occur at the right time. It assumes that the transition occurs
    // at the start of the pipeline, but we haven’t acquired the image yet at that point because the Renderer uses
    // eColorAttachmentOutput for the drawing wait stage. There are two ways to deal with this problem:
    // - In the Renderer, change the waitStages for the imageAvailableSemaphore to eTopOfPipe to ensure that the render
    // passes don’t begin until the image is available
    // - Make the render pass wait for the eColorAttachmentOutput stage, using subpass dependency
    using Stage = vk::PipelineStageFlagBits;
    using Access = vk::AccessFlagBits;
    constexpr auto srcStages = Stage::eColorAttachmentOutput | Stage::eLateFragmentTests;
    constexpr auto dstStages = Stage::eColorAttachmentOutput | Stage::eEarlyFragmentTests;
    constexpr auto srcAccess = Access::eColorAttachmentWrite | Access::eDepthStencilAttachmentWrite;
    constexpr auto dstAccess = Access::eColorAttachmentWrite | Access::eDepthStencilAttachmentWrite;
    constexpr auto dependency = vk::SubpassDependency{
        vk::SubpassExternal,  // src subpass
        0,                    // dst subpass
        srcStages, dstStages,
        srcAccess, dstAccess,
    };

    const auto renderPassInfo = vk::RenderPassCreateInfo{ {}, attachments, subpass, dependency };
    _renderPass = _device.createRenderPass(renderPassInfo);
}

void tpd::renderer::ForwardRenderer::createFramebuffers() {
    const auto toFramebuffer = [this](const auto& swapChainImageView) {
        // Specify the image views that should be bound to the respective attachment descriptions in the render pass
        const auto attachments = std::array{ _framebufferColorImageView, _framebufferDepthImageView, swapChainImageView };

        auto framebufferInfo = vk::FramebufferCreateInfo{};
        framebufferInfo.renderPass = _renderPass;
        framebufferInfo.attachmentCount = attachments.size();
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = _swapChainImageExtent.width;
        framebufferInfo.height = _swapChainImageExtent.height;
        framebufferInfo.layers = 1;  // our swap chain images are single images, so the number of layers is 1

        return _device.createFramebuffer(framebufferInfo);
    };

    // The swap chain attachment differs for every swap chain image, but the same color and depth image can be used by
    // all of them because only a single subpass is running at the same time due to our semaphores.
    _framebuffers = _swapChainImageViews | std::ranges::views::transform(toFramebuffer) | std::ranges::to<std::vector>();
}

vk::GraphicsPipelineCreateInfo tpd::renderer::ForwardRenderer::getGraphicsPipelineInfo() const {
    // Specify which properties will be able to change at runtime
    static constexpr auto dynamicStates = std::array{
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
        vk::DynamicState::eVertexInputEXT,
        vk::DynamicState::ePrimitiveTopology,
        vk::DynamicState::ePolygonModeEXT,
        vk::DynamicState::eCullMode,
        vk::DynamicState::eLineWidth,
        vk::DynamicState::eRasterizationSamplesEXT,
    };

    static constexpr auto dynamicStateInfo = vk::PipelineDynamicStateCreateInfo{ {}, dynamicStates.size(), dynamicStates.data() };
    static constexpr auto vertexInputState = vk::PipelineVertexInputStateCreateInfo{};
    static constexpr auto viewportState = vk::PipelineViewportStateCreateInfo{ {}, 1, {}, 1, {} };
    static constexpr auto inputAssembly = vk::PipelineInputAssemblyStateCreateInfo{ {}, vk::PrimitiveTopology::eTriangleList, vk::False };
    static constexpr auto depthStencil  = vk::PipelineDepthStencilStateCreateInfo{ {}, vk::True, vk::True, vk::CompareOp::eLess };

    static constexpr auto rasterization = vk::PipelineRasterizationStateCreateInfo{
        {}, vk::False, vk::False, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack,
        vk::FrontFace::eCounterClockwise, vk::False, {}, {}, {}, /* line width */ 1.0f };

    static auto colorBlendAttachment = vk::PipelineColorBlendAttachmentState{ vk::False, };
    colorBlendAttachment.blendEnable = vk::False;
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR |
                                          vk::ColorComponentFlagBits::eG |
                                          vk::ColorComponentFlagBits::eB |
                                          vk::ColorComponentFlagBits::eA;
    static const auto colorBlending = vk::PipelineColorBlendStateCreateInfo{ {}, vk::False, {}, colorBlendAttachment };
    static const auto multisampling = vk::PipelineMultisampleStateCreateInfo{
        {}, /* MSAA */ _msaaSampleCount, /* Sample Shading */ vk::True, _minSampleShading };

    auto pipelineInfo = vk::GraphicsPipelineCreateInfo{};
    pipelineInfo.pDynamicState       = &dynamicStateInfo;
    pipelineInfo.pVertexInputState   = &vertexInputState;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pDepthStencilState  = &depthStencil;
    pipelineInfo.pRasterizationState = &rasterization;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pMultisampleState   = &multisampling;
    // Reference to the render pass and the index of the subpass where this graphics pipeline will be used.
    // It is also possible to use other render passes with this pipeline instead of this specific instance,
    // but they have to be compatible with this very specific renderPass
    pipelineInfo.renderPass = _renderPass;
    pipelineInfo.subpass = 0;
    // Pipeline derivatives can be used to create multiple pipelines that share most of their state
    pipelineInfo.basePipelineHandle = nullptr;
    pipelineInfo.basePipelineIndex = -1;

    return pipelineInfo;
}

void tpd::renderer::ForwardRenderer::onDraw(const vk::CommandBuffer buffer) {
}
