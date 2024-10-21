#include "torpedo/material/PhongMaterial.h"

std::unique_ptr<tpd::PhongMaterial> tpd::PhongMaterial::Builder::build(const Renderer& renderer) const {
    auto shaderLayout = getSharedLayoutBuilder(1)
        .descriptor(1, 0, vk::DescriptorType::eUniformBuffer,        1, vk::ShaderStageFlagBits::eFragment)
        .descriptor(1, 1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment)
        .descriptor(1, 2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment)
        .build(renderer.getVulkanDevice());

    return Material::Builder()
        .vertShader(getShaderFilePath("graphics.vert.spv"))
        .fragShader(getShaderFilePath("phong.frag.spv"))
        .minSampleShading(_minSampleShading)
        .build<PhongMaterial>(renderer, std::move(shaderLayout));
}

std::shared_ptr<tpd::PhongMaterialInstance> tpd::PhongMaterial::createInstance(const Renderer& renderer) const {
    auto shaderInstance = _shaderLayout->createInstance(_device, Renderer::MAX_FRAMES_IN_FLIGHT, 1);
    return std::make_shared<PhongMaterialInstance>(renderer, std::move(shaderInstance), this);
}
