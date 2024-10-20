#include "torpedo/graphics/MaterialInstance.h"

#include "torpedo/graphics/Material.h"
#include "torpedo/graphics/Renderer.h"

void tpd::MaterialInstance::bindDescriptorSets(const vk::CommandBuffer buffer, const uint32_t frameIndex) const {
    // Only bind if the number of descriptor sets are greater than 0, otherwise the validation layer will complain
    if (!_shaderInstance->empty()) {
        buffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, _material->getVulkanPipelineLayout(),
            _firstSet, _shaderInstance->getDescriptorSets(frameIndex), {});
    }
}

void tpd::MaterialInstance::dispose() noexcept {
    if (_shaderInstance) {
        _shaderInstance->dispose(_material->getVulkanDevice());
        _shaderInstance.reset();
    }
}

tpd::MaterialInstance::~MaterialInstance() {
    dispose();
    _material = nullptr;
}
