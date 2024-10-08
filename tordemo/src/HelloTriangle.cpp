#include "HelloTriangle.h"

void HelloTriangle::onInitialized() {
    createPipeline();
    createBuffers();
}

void HelloTriangle::createPipeline() {
    // We don't need to create a descriptor set layout for this simple example
    _pipelineLayout = _device.createPipelineLayout({});

    // Shaders
    const auto vertShaderModule = createShaderModule(readShaderFile("assets/shaders/simple.vert.spv"));
    const auto fragShaderModule = createShaderModule(readShaderFile("assets/shaders/simple.frag.spv"));
    const auto shaderStageInfos = std::array{
        vk::PipelineShaderStageCreateInfo{ {}, vk::ShaderStageFlagBits::eVertex,   vertShaderModule, "main" },
        vk::PipelineShaderStageCreateInfo{ {}, vk::ShaderStageFlagBits::eFragment, fragShaderModule, "main" },
    };

    // Specify which properties will be able to change at runtime
    constexpr auto dynamicStates = std::array{ vk::DynamicState::eViewport, vk::DynamicState::eScissor };
    auto dynamicStateInfo = vk::PipelineDynamicStateCreateInfo{};
    dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicStateInfo.pDynamicStates = dynamicStates.data();

    // Default viewport state
    const auto viewportState = getPipelineDefaultViewportState();
    // Describe what kind of geometry will be drawn from the vertices and if primitive restart should be enabled
    const auto inputAssembly = getPipelineDefaultInputAssemblyState();
    // Input vertex binding and attribute descriptions
    const auto vertexInputState = getVertexInputState();
    // Rasterizer options
    const auto rasterizer = getPipelineDefaultRasterizationState();
    // MSAA
    const auto multisampling = getMultisampleState();
    // Depth stencil options
    const auto depthStencil = getPipelineDefaultDepthStencilState();
    // How the newly computed fragment colors are combined with the color that is already in the framebuffer
    const auto colorBlendAttachment = getPipelineDefaultColorBlendAttachmentState();
    // Configure global color blending
    const auto colorBlending = vk::PipelineColorBlendStateCreateInfo{ {}, vk::False, {}, 1, &colorBlendAttachment };

    // Construct the pipeline
    auto pipelineInfo = vk::GraphicsPipelineCreateInfo{};
    // Pipeline layout
    pipelineInfo.layout = _pipelineLayout;
    // Shader stages
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStageInfos.size());
    pipelineInfo.pStages = shaderStageInfos.data();
    // Fixed and dynamic states
    pipelineInfo.pVertexInputState = &vertexInputState;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicStateInfo;
    // Reference to the render pass and the index of the subpass where this graphics pipeline will be used.
    // It is also possible to use other render passes with this pipeline instead of this specific instance,
    // but they have to be compatible with this very specific renderPass
    pipelineInfo.renderPass = _renderPass;
    pipelineInfo.subpass = 0;
    // Pipeline derivatives can be used to create multiple pipelines that share most of their state
    pipelineInfo.basePipelineHandle = nullptr;
    pipelineInfo.basePipelineIndex = -1;

    // Create the graphics pipeline
    _graphicsPipeline = _device.createGraphicsPipeline(nullptr, pipelineInfo).value;

    // We no longer need the shader modules once the pipeline is created
    _device.destroyShaderModule(vertShaderModule);
    _device.destroyShaderModule(fragShaderModule);
}

vk::PipelineVertexInputStateCreateInfo HelloTriangle::getVertexInputState() {
    static constexpr auto bindingDescriptions = std::array{
        vk::VertexInputBindingDescription{ 0, 3 * sizeof(float) },
        vk::VertexInputBindingDescription{ 1, 4 * sizeof(float) },
    };
    static constexpr auto attributeDescriptions = std::array{
        vk::VertexInputAttributeDescription{ 0, 0, vk::Format::eR32G32B32Sfloat },
        vk::VertexInputAttributeDescription{ 1, 1, vk::Format::eR32G32B32A32Sfloat },
    };

    auto vertexInputStateCreateInfo = vk::PipelineVertexInputStateCreateInfo{};
    vertexInputStateCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
    vertexInputStateCreateInfo.pVertexBindingDescriptions = bindingDescriptions.data();
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputStateCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    return vertexInputStateCreateInfo;
}

vk::PipelineMultisampleStateCreateInfo HelloTriangle::getMultisampleState() const {
    // Use MSAA for the framebuffer's color attachment
    auto multisampleStateCreateInfo = vk::PipelineMultisampleStateCreateInfo{};
    multisampleStateCreateInfo.rasterizationSamples = getFramebufferColorImageSampleCount();
    multisampleStateCreateInfo.sampleShadingEnable = vk::False;
    multisampleStateCreateInfo.alphaToCoverageEnable = vk::False;
    multisampleStateCreateInfo.alphaToOneEnable = vk::False;
    return multisampleStateCreateInfo;
}

void HelloTriangle::createBuffers() {
    constexpr auto vertices = std::array{
         0.0f, -0.5f, 0.0f,  // 1st vertex
        -0.5f,  0.5f, 0.0f,  // 2nd vertex
         0.5f,  0.5f, 0.0f,  // 3rd vertex
    };
    constexpr auto colors = std::array{
        1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 1.0f,
    };

    const auto bufferCopy = [this](const vk::Buffer src, const vk::Buffer dst, const vk::BufferCopy& copyInfo) {
        const auto cmdBuffer = beginSingleTimeTransferCommands();
        cmdBuffer.copyBuffer(src, dst, copyInfo);
        endSingleTimeTransferCommands(cmdBuffer);
    };

    _vertexBuffer = tpd::Buffer::Builder()
        .bufferCount(2)
        .usage(vk::BufferUsageFlagBits::eVertexBuffer)
        .buffer(0, 3 * sizeof(float) * vertices.size())
        .buffer(1, 4 * sizeof(float) * colors.size())
        .build(_allocator, tpd::ResourceType::Dedicated);
    _vertexBuffer->transferBufferData(0, vertices.data(), _allocator, bufferCopy);
    _vertexBuffer->transferBufferData(1, colors.data(),   _allocator, bufferCopy);

    static constexpr auto indices = std::array<uint16_t, 3>{ 0, 1, 2 };

    _indexBuffer = tpd::Buffer::Builder()
        .bufferCount(1)
        .usage(vk::BufferUsageFlagBits::eIndexBuffer)
        .buffer(0, sizeof(uint16_t) * indices.size())
        .build(_allocator, tpd::ResourceType::Dedicated);
    _indexBuffer->transferBufferData(0, indices.data(), _allocator, bufferCopy);
}

void HelloTriangle::onDraw(const vk::CommandBuffer buffer) {
    buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _graphicsPipeline);

    // Set viewport and scissor states
    auto viewport = vk::Viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(_swapChainImageExtent.width);
    viewport.height = static_cast<float>(_swapChainImageExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    buffer.setViewport(0, viewport);
    buffer.setScissor(0, vk::Rect2D{ { 0, 0 }, _swapChainImageExtent });

    const auto vertexBuffers = std::vector(_vertexBuffer->getBufferCount(), _vertexBuffer->getBuffer());
    buffer.bindVertexBuffers(0, vertexBuffers, _vertexBuffer->getOffsets());

    buffer.bindIndexBuffer(_indexBuffer->getBuffer(), 0, vk::IndexType::eUint16);

    buffer.drawIndexed(
        /* index count    */ 3,
        /* instance count */ 1,
        /* first index    */ 0,
        /* vertex offset  */ 0,
        /* first instance */ 0);
}

HelloTriangle::~HelloTriangle() {
    _vertexBuffer->dispose(_allocator);
    _indexBuffer->dispose(_allocator);
    _device.destroyPipeline(_graphicsPipeline);
    _device.destroyPipelineLayout(_pipelineLayout);
}
