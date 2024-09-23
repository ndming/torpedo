#include "TexturedCube.h"
#include "stb.h"

#include <torpedo/bootstrap/SamplerBuilder.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void TexturedCube::createPipelineResources() {
    TordemoApplication::createPipelineResources();
    createDrawingBuffers();
    createUniformBuffer();
    createTextureResources();
    createPipelineInstance();
}

void TexturedCube::createDrawingBuffers() {
    static constexpr auto vertices = std::array{
        Vertex{ {  1.0f, -1.0f,  1.0f, }, { 1.0f, 0.0f, 1.0f, 1.0f, }, { 0.0f, 0.0f, } },
        Vertex{ {  1.0f, -1.0f, -1.0f, }, { 1.0f, 0.0f, 0.0f, 1.0f, }, { 0.0f, 1.0f, } },
        Vertex{ {  1.0f,  1.0f,  1.0f, }, { 1.0f, 1.0f, 1.0f, 1.0f, }, { 1.0f, 0.0f, } },
        Vertex{ {  1.0f,  1.0f, -1.0f, }, { 1.0f, 1.0f, 0.0f, 1.0f, }, { 1.0f, 1.0f, } },
        Vertex{ {  1.0f,  1.0f,  1.0f, }, { 1.0f, 1.0f, 1.0f, 1.0f, }, { 0.0f, 0.0f, } },
        Vertex{ {  1.0f,  1.0f, -1.0f, }, { 1.0f, 1.0f, 0.0f, 1.0f, }, { 0.0f, 1.0f, } },
        Vertex{ { -1.0f,  1.0f,  1.0f, }, { 0.0f, 1.0f, 1.0f, 1.0f, }, { 1.0f, 0.0f, } },
        Vertex{ { -1.0f,  1.0f, -1.0f, }, { 0.0f, 1.0f, 0.0f, 1.0f, }, { 1.0f, 1.0f, } },
        Vertex{ { -1.0f,  1.0f,  1.0f, }, { 0.0f, 1.0f, 1.0f, 1.0f, }, { 0.0f, 0.0f, } },
        Vertex{ { -1.0f, -1.0f,  1.0f, }, { 0.0f, 0.0f, 1.0f, 1.0f, }, { 0.0f, 1.0f, } },
        Vertex{ {  1.0f,  1.0f,  1.0f, }, { 1.0f, 1.0f, 1.0f, 1.0f, }, { 1.0f, 0.0f, } },
        Vertex{ {  1.0f, -1.0f,  1.0f, }, { 1.0f, 0.0f, 1.0f, 1.0f, }, { 1.0f, 1.0f, } },
        Vertex{ { -1.0f,  1.0f,  1.0f, }, { 0.0f, 1.0f, 1.0f, 1.0f, }, { 0.0f, 0.0f, } },
        Vertex{ { -1.0f,  1.0f, -1.0f, }, { 0.0f, 1.0f, 0.0f, 1.0f, }, { 0.0f, 1.0f, } },
        Vertex{ { -1.0f, -1.0f,  1.0f, }, { 0.0f, 0.0f, 1.0f, 1.0f, }, { 1.0f, 0.0f, } },
        Vertex{ { -1.0f, -1.0f, -1.0f, }, { 0.0f, 0.0f, 0.0f, 1.0f, }, { 1.0f, 1.0f, } },
        Vertex{ { -1.0f, -1.0f,  1.0f, }, { 0.0f, 0.0f, 1.0f, 1.0f, }, { 0.0f, 0.0f, } },
        Vertex{ { -1.0f, -1.0f, -1.0f, }, { 0.0f, 0.0f, 0.0f, 1.0f, }, { 0.0f, 1.0f, } },
        Vertex{ {  1.0f, -1.0f,  1.0f, }, { 1.0f, 0.0f, 1.0f, 1.0f, }, { 1.0f, 0.0f, } },
        Vertex{ {  1.0f, -1.0f, -1.0f, }, { 1.0f, 0.0f, 0.0f, 1.0f, }, { 1.0f, 1.0f, } },
        Vertex{ {  1.0f,  1.0f, -1.0f, }, { 1.0f, 1.0f, 0.0f, 1.0f, }, { 0.0f, 0.0f, } },
        Vertex{ {  1.0f, -1.0f, -1.0f, }, { 1.0f, 0.0f, 0.0f, 1.0f, }, { 0.0f, 1.0f, } },
        Vertex{ { -1.0f,  1.0f, -1.0f, }, { 0.0f, 1.0f, 0.0f, 1.0f, }, { 1.0f, 0.0f, } },
        Vertex{ { -1.0f, -1.0f, -1.0f, }, { 0.0f, 0.0f, 0.0f, 1.0f, }, { 1.0f, 1.0f, } },
    };

    for (auto i = 0; i < 6; ++i) {
        const auto it = i * 4;
        _indices.push_back(it);     _indices.push_back(it + 1); _indices.push_back(it + 2);
        _indices.push_back(it + 2); _indices.push_back(it + 1); _indices.push_back(it + 3);
    }

    _vertexBuffer = tpd::Buffer::Builder()
        .bufferCount(1)
        .usage(vk::BufferUsageFlagBits::eVertexBuffer)
        .buffer(0, sizeof(Vertex) * vertices.size())
        .buildDedicated(_resourceAllocator);

    _indexBuffer = tpd::Buffer::Builder()
        .bufferCount(1)
        .usage(vk::BufferUsageFlagBits::eIndexBuffer)
        .buffer(0, sizeof(uint16_t) * _indices.size())
        .buildDedicated(_resourceAllocator);

    const auto bufferCopy = [this](const vk::Buffer src, const vk::Buffer dst, const vk::BufferCopy& copyInfo) {
        const auto cmdBuffer = beginSingleTimeTransferCommands();
        cmdBuffer.copyBuffer(src, dst, copyInfo);
        endSingleTimeTransferCommands(cmdBuffer);
    };

    _vertexBuffer->transferBufferData(0, vertices.data(), _resourceAllocator, bufferCopy);
    _indexBuffer->transferBufferData(0, _indices.data(), _resourceAllocator, bufferCopy);
}

void TexturedCube::createUniformBuffer() {
    auto uniformBufferBuilder = tpd::Buffer::Builder()
        .bufferCount(_maxFramesInFlight)
        .usage(vk::BufferUsageFlagBits::eUniformBuffer);
    const auto alignment = _physicalDevice.getProperties().limits.minUniformBufferOffsetAlignment;
    for (uint32_t i = 0; i < _maxFramesInFlight; ++i) { uniformBufferBuilder.buffer(i, sizeof(MVP), alignment); }
    _uniformBuffer = uniformBufferBuilder.buildPersistent(_resourceAllocator);
}

void TexturedCube::createTextureResources() {
    int texWidth, texHeight, texChannels;
    const auto pixels = stbi_load("assets/textures/owl.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    const auto imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("Failed to load texture image!");
    }

    _texture = tpd::Image::Builder()
        .dimensions(texWidth, texHeight, 1)
        .type(vk::ImageType::e2D)
        .format(vk::Format::eR8G8B8A8Srgb)
        .usage(vk::ImageUsageFlagBits::eSampled)
        .imageViewType(vk::ImageViewType::e2D)
        .imageViewAspects(vk::ImageAspectFlagBits::eColor)
        .buildDedicated(_resourceAllocator, _device);

    const auto layoutTransition = [this](const vk::ImageLayout oldLayout, const vk::ImageLayout newLayout, const vk::Image image) {
        const auto commandBuffer = beginSingleTimeTransferCommands();

        auto barrier = vk::ImageMemoryBarrier{};
        barrier.image = image;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.subresourceRange = { vk::ImageAspectFlagBits::eColor, /* base mip */ 0, /* level count */ 1, 0, 1 };
        // We're not using the barrier for transferring queue family ownership.
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;

        vk::PipelineStageFlags srcStage;
        vk::PipelineStageFlags dstStage;

        using enum vk::ImageLayout;
        if (oldLayout == eUndefined && newLayout == eTransferDstOptimal) {
            // Transfer writes that don’t need to wait on anything, we specify an empty access mask and the earliest
            // possible pipeline stage: eTopOfPipe
            barrier.srcAccessMask = static_cast<vk::AccessFlags>(0);
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
            srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
            dstStage = vk::PipelineStageFlagBits::eTransfer;
        } else if (oldLayout == eTransferDstOptimal && newLayout == eShaderReadOnlyOptimal) {
            // Shader reads should wait on transfer writes.
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
            // The image will be written in the same pipeline stage and subsequently read by a shader
            srcStage = vk::PipelineStageFlagBits::eTransfer;
            dstStage = vk::PipelineStageFlagBits::eFragmentShader;
        } else {
            throw std::invalid_argument("Unsupported layout transition!");
        }

        commandBuffer.pipelineBarrier(
            srcStage, dstStage,
            static_cast<vk::DependencyFlags>(0),
            {},      // for memory barriers
            {},      // for buffer memory barriers
            barrier  // for image memory barriers
        );
        endSingleTimeTransferCommands(commandBuffer);
    };

    const auto bufferToImageCopy = [this, &texWidth, &texHeight](const vk::Buffer buffer, const vk::Image image) {
        const auto commandBuffer = beginSingleTimeTransferCommands();

        auto region = vk::BufferImageCopy{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageOffset = vk::Offset3D{ 0, 0, 0 };
        region.imageExtent = vk::Extent3D{ static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1 };
        region.imageSubresource = { vk::ImageAspectFlagBits::eColor, /* mip level */ 0, 0, 1 };

        commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
        endSingleTimeTransferCommands(commandBuffer);
    };

    _texture->transferImageData(
        pixels, imageSize, vk::ImageLayout::eShaderReadOnlyOptimal,
        _resourceAllocator, layoutTransition, bufferToImageCopy);

    stbi_image_free(pixels);

    _sampler = tpd::SamplerBuilder().build(_device);
}

vk::PipelineVertexInputStateCreateInfo TexturedCube::getGraphicsPipelineVertexInputState() const {
    static constexpr auto bindingDescriptions = std::array{
        vk::VertexInputBindingDescription{ 0, sizeof(Vertex) },
    };
    static constexpr auto attributeDescriptions = std::array{
        vk::VertexInputAttributeDescription{ 0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position) },
        vk::VertexInputAttributeDescription{ 1, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, color) },
        vk::VertexInputAttributeDescription{ 2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord) },
    };

    auto vertexInputStateCreateInfo = vk::PipelineVertexInputStateCreateInfo{};
    vertexInputStateCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
    vertexInputStateCreateInfo.pVertexBindingDescriptions = bindingDescriptions.data();
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputStateCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    return vertexInputStateCreateInfo;
}

std::unique_ptr<tpd::PipelineShader> TexturedCube::buildPipelineShader(vk::GraphicsPipelineCreateInfo* pipelineInfo) const {
    return tpd::PipelineShader::Builder(1, 2)
        .shader("assets/shaders/textured.vert.spv", vk::ShaderStageFlagBits::eVertex)
        .shader("assets/shaders/textured.frag.spv", vk::ShaderStageFlagBits::eFragment)
        .descriptor(0, 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex)
        .descriptor(0, 1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment)
        .build(pipelineInfo, _physicalDevice, _device);
}

void TexturedCube::createPipelineInstance() {
    _pipelineInstance = _graphicsPipelineShader->createInstance(_device, _maxFramesInFlight);

    for (uint32_t i = 0; i < _maxFramesInFlight; ++i) {
        auto bufferInfo = vk::DescriptorBufferInfo{};
        bufferInfo.buffer = _uniformBuffer->getBuffer();
        bufferInfo.offset = _uniformBuffer->getOffsets()[i];
        bufferInfo.range = sizeof(MVP);

        auto imageInfo = vk::DescriptorImageInfo{};
        imageInfo.imageView = _texture->getImageView();
        imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageInfo.sampler = _sampler;

        _pipelineInstance->setDescriptors(i, 0, 0, vk::DescriptorType::eUniformBuffer, _device, { bufferInfo });
        _pipelineInstance->setDescriptors(i, 0, 1, vk::DescriptorType::eCombinedImageSampler, _device, { imageInfo });
    }
}

void TexturedCube::onFrameReady() {
    TordemoApplication::onFrameReady();

    static auto startTime = std::chrono::high_resolution_clock::now();
    const auto currentTime = std::chrono::high_resolution_clock::now();
    const float frameTime = std::chrono::duration<float>(currentTime - startTime).count();

    auto mvp = MVP{};
    const auto model = rotate(glm::mat4(1.0f), frameTime * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    std::copy_n(value_ptr(model), 16, mvp.model.begin());
    const auto view = lookAt(glm::vec3(4.0f, 4.0f, 4.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    std::copy_n(value_ptr(view), 16, mvp.view.begin());
    const auto ratio = static_cast<float>(_swapChainImageExtent.width) / static_cast<float>(_swapChainImageExtent.height);
    auto proj = glm::perspective(glm::radians(45.0f), ratio, 0.1f, 10.0f); proj[1][1] *= -1.0f;
    std::copy_n(value_ptr(proj), 16, mvp.proj.begin());

    _uniformBuffer->updateBufferData(_currentFrame, &mvp, sizeof(MVP));
}

void TexturedCube::render(const vk::CommandBuffer buffer) {
    TordemoApplication::render(buffer);

    buffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics, _graphicsPipelineShader->getPipelineLayout(),
        /* first set */ 0, _pipelineInstance->getDescriptorSets(_currentFrame), {});

    const auto vertexBuffers = std::vector(_vertexBuffer->getBufferCount(), _vertexBuffer->getBuffer());
    buffer.bindVertexBuffers(0, vertexBuffers, _vertexBuffer->getOffsets());

    buffer.bindIndexBuffer(_indexBuffer->getBuffer(), 0, vk::IndexType::eUint16);
    buffer.drawIndexed(static_cast<uint32_t>(_indices.size()), 1, 0, 0, 0);
}

TexturedCube::~TexturedCube() {
    _pipelineInstance->destroy(_device);
    _device.destroySampler(_sampler);
    _texture->destroy(_device, _resourceAllocator);
    _uniformBuffer->destroy(_resourceAllocator);
    _indexBuffer->destroy(_resourceAllocator);
    _vertexBuffer->destroy(_resourceAllocator);
}
