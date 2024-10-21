#include "torpedo/material/PhongMaterialInstance.h"

#include "torpedo/graphics/Material.h"

tpd::PhongMaterialInstance::PhongMaterialInstance(
    const Renderer& renderer,
    std::unique_ptr<ShaderInstance> shaderInstance,
    const Material* material)
    : MaterialInstance{ std::move(shaderInstance), material }
{
    auto materialObjectBufferBuilder = Buffer::Builder()
        .bufferCount(Renderer::MAX_FRAMES_IN_FLIGHT)
        .usage(vk::BufferUsageFlagBits::eUniformBuffer);
    const auto alignment = renderer.getMinUniformBufferOffsetAlignment();
    for (uint32_t i = 0; i < Renderer::MAX_FRAMES_IN_FLIGHT; ++i) {
        materialObjectBufferBuilder.buffer(i, sizeof(MaterialObject), alignment);
    }
    _materialObjectBuffer = materialObjectBufferBuilder.build(renderer.getResourceAllocator(), ResourceType::Persistent);

    for (uint32_t i = 0; i < Renderer::MAX_FRAMES_IN_FLIGHT; ++i) {
        auto bufferInfo = vk::DescriptorBufferInfo{};
        bufferInfo.buffer = _materialObjectBuffer->getBuffer();
        bufferInfo.offset = _materialObjectBuffer->getOffsets()[i];
        bufferInfo.range = sizeof(MaterialObject);

        // Set a dummy image to the combined image sampler descriptors, otherwise, the validation layer will complain
        auto imageInfo = vk::DescriptorImageInfo{};
        imageInfo.imageView = _dummyImage->getImageView();
        imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageInfo.sampler = Texture::getDefaultSampler();

        // Although MaterialObject is at descriptor set 1, we didn't include set 0 when we create the ShaderInstance.
        // Therefore, MaterialObject is now the first set (set 0) in the ShaderInstance.
        _shaderInstance->setDescriptors(i, 0, 0, vk::DescriptorType::eUniformBuffer,        material->getVulkanDevice(), { bufferInfo });
        _shaderInstance->setDescriptors(i, 0, 1, vk::DescriptorType::eCombinedImageSampler, material->getVulkanDevice(), { imageInfo });
        _shaderInstance->setDescriptors(i, 0, 2, vk::DescriptorType::eCombinedImageSampler, material->getVulkanDevice(), { imageInfo });
    }
}

void tpd::PhongMaterialInstance::activate(const vk::CommandBuffer buffer, const uint32_t frameIndex) const {
    _materialObjectBuffer->updateBufferData(frameIndex, &_materialObject, sizeof(MaterialObject));
    buffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics, _material->getVulkanPipelineLayout(),
        1, _shaderInstance->getDescriptorSets(frameIndex), {});
}

void tpd::PhongMaterialInstance::setDiffuse(const Texture& texture) {
    for (uint32_t i = 0; i < Renderer::MAX_FRAMES_IN_FLIGHT; ++i) {
        auto imageInfo = vk::DescriptorImageInfo{};
        imageInfo.imageView = texture.getImage()->getImageView();
        imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageInfo.sampler = texture.getSampler();
        _shaderInstance->setDescriptors(i, 0, 1, vk::DescriptorType::eCombinedImageSampler, _material->getVulkanDevice(), { imageInfo });
    }
    _materialObject.useDiffuseMap = 1;
}

void tpd::PhongMaterialInstance::setSpecular(const Texture& texture) {
    for (uint32_t i = 0; i < Renderer::MAX_FRAMES_IN_FLIGHT; ++i) {
        auto imageInfo = vk::DescriptorImageInfo{};
        imageInfo.imageView = texture.getImage()->getImageView();
        imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageInfo.sampler = texture.getSampler();
        _shaderInstance->setDescriptors(i, 0, 2, vk::DescriptorType::eCombinedImageSampler, _material->getVulkanDevice(), { imageInfo });
    }
    _materialObject.useSpecularMap = 1;
}

void tpd::PhongMaterialInstance::dispose() noexcept {
    if (_allocator && _materialObjectBuffer) {
        _materialObjectBuffer->dispose(*_allocator);
        _materialObjectBuffer.reset();
    }
    MaterialInstance::dispose();
}

tpd::PhongMaterialInstance::~PhongMaterialInstance() {
    dispose();
}
