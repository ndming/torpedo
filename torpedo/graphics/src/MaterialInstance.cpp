#include "torpedo/graphics/MaterialInstance.h"

#include "torpedo/graphics/Material.h"
#include "torpedo/graphics/Renderer.h"

void tpd::MaterialInstance::activate(const vk::CommandBuffer buffer, const uint32_t frameIndex) const {
    // Only bind if the number of descriptor sets are greater than 0, otherwise the validation layer will complain
    if (!_shaderInstance->empty()) {
        // If this Material was not created with user-declared ShaderLayout, the first descriptor set (shared set)
        // has already been created and managed by the Renderer. The boolean returned by builtWithSharedLayout()
        // indicates if the Material was created with shared layout or not. If it was, firstSet will get a value of 1.
        const auto firstSet = static_cast<uint32_t>(_material->builtWithSharedLayout());
        buffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, _material->getVulkanPipelineLayout(),
            firstSet, _shaderInstance->getDescriptorSets(frameIndex), {});
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
