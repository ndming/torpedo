#include "torpedo/rendering/HeadlessRenderer.h"

#include <plog/Log.h>

void tpd::HeadlessRenderer::init(const uint32_t framebufferWidth, const uint32_t framebufferHeight) {
    if (!_initialized) [[likely]] {
        PLOGI << "Initializing renderer: OffScreenRenderer";
        _framebufferSize = vk::Extent2D{ framebufferWidth, framebufferHeight };
        Renderer::init();
    } else {
        PLOGI << "Skipping already initialized renderer: OffScreenRenderer";
    }
}
