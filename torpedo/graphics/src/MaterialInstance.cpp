#include "torpedo/graphics/MaterialInstance.h"

void tpd::MaterialInstance::dispose(const Renderer& renderer) noexcept {
    _shaderInstance->dispose(renderer.getVulkanDevice());
    _material = nullptr;
}
