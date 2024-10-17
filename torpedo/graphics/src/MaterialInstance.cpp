#include "torpedo/graphics/MaterialInstance.h"

#include "torpedo/graphics/Material.h"
#include "torpedo/graphics/Renderer.h"

void tpd::MaterialInstance::bindDescriptorSets(const vk::CommandBuffer buffer, const uint32_t frameIndex) const {
    if (!_shaderInstance->empty()) {
        buffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics, _material->getVulkanPipelineLayout(),
        /* first set */ 0, _shaderInstance->getDescriptorSets(frameIndex), {});
    }
}

void tpd::MaterialInstance::dispose(const Renderer& renderer) noexcept {
    _shaderInstance->dispose(renderer.getVulkanDevice());
    _material = nullptr;
}
