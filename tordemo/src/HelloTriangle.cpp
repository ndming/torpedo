#include "HelloTriangle.h"

#include <glm/glm.hpp>

void HelloTriangle::createPipelineResources() {
    TordemoApplication::createPipelineResources();
    createBuffers();
}

void HelloTriangle::createBuffers() {
    static constexpr auto vertices = std::array{
        glm::vec3{  0.0f, -0.5f, 0.0f },
        glm::vec3{ -0.5f,  0.5f, 0.0f },
        glm::vec3{  0.5f,  0.5f, 0.0f },
    };
    static constexpr auto colors = std::array{
        glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f },
        glm::vec4{ 0.0f, 1.0f, 0.0f, 1.0f },
        glm::vec4{ 0.0f, 0.0f, 1.0f, 1.0f },
    };

    const auto bufferCopy = [this](const vk::Buffer src, const vk::Buffer dst, const vk::BufferCopy& copyInfo) {
        const auto cmdBuffer = beginSingleTimeTransferCommands();
        cmdBuffer.copyBuffer(src, dst, copyInfo);
        endSingleTimeTransferCommands(cmdBuffer);
    };

    _vertexBuffer = tpd::Buffer::Builder()
        .bufferCount(2)
        .usage(vk::BufferUsageFlagBits::eVertexBuffer)
        .buffer(0, sizeof(glm::vec3) * vertices.size())
        .buffer(1, sizeof(glm::vec4) * colors.size())
        .buildDedicated(_resourceAllocator);
    _vertexBuffer->transferBufferData(0, vertices.data(), _resourceAllocator, bufferCopy);
    _vertexBuffer->transferBufferData(1, colors.data(),   _resourceAllocator, bufferCopy);

    static constexpr auto indices = std::array<uint16_t, 3>{ 0, 1, 2 };

    _indexBuffer = tpd::Buffer::Builder()
        .bufferCount(1)
        .usage(vk::BufferUsageFlagBits::eIndexBuffer)
        .buffer(0, sizeof(uint16_t) * indices.size())
        .buildDedicated(_resourceAllocator);
    _indexBuffer->transferBufferData(0, indices.data(), _resourceAllocator, bufferCopy);
}

vk::PipelineVertexInputStateCreateInfo HelloTriangle::getGraphicsPipelineVertexInputState() const {
    static constexpr auto bindingDescriptions = std::array{
        vk::VertexInputBindingDescription{ 0, sizeof(glm::vec3) },
        vk::VertexInputBindingDescription{ 1, sizeof(glm::vec4) },
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

std::unique_ptr<tpd::PipelineShader> HelloTriangle::buildPipelineShader(vk::GraphicsPipelineCreateInfo* pipelineInfo) const {
    return tpd::PipelineShader::Builder()
        .shader("assets/shaders/simple.vert.spv", vk::ShaderStageFlagBits::eVertex)
        .shader("assets/shaders/simple.frag.spv", vk::ShaderStageFlagBits::eFragment)
        .build(pipelineInfo, _physicalDevice, _device);
}

void HelloTriangle::render(const vk::CommandBuffer buffer) {
    TordemoApplication::render(buffer);

    const auto vertexBuffers = std::vector(_vertexBuffer->getBufferCount(), static_cast<vk::Buffer>(*_vertexBuffer));
    buffer.bindVertexBuffers(0, vertexBuffers, _vertexBuffer->getOffsets());

    buffer.bindIndexBuffer(static_cast<vk::Buffer>(*_indexBuffer), 0, vk::IndexType::eUint16);

    buffer.drawIndexed(
        /* index count    */ 3,
        /* instance count */ 1,
        /* first index    */ 0,
        /* vertex offset  */ 0,
        /* first instance */ 0);
}

HelloTriangle::~HelloTriangle() {
    _vertexBuffer->destroy(_resourceAllocator);
    _indexBuffer->destroy(_resourceAllocator);
}
