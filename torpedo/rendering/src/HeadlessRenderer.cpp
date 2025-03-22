#include "torpedo/rendering/HeadlessRenderer.h"

#include <plog/Log.h>

void tpd::HeadlessRenderer::init(const uint32_t frameWidth, const uint32_t frameHeight, std::pmr::memory_resource* contextPool) {
    if (_initialized) [[unlikely]] {
        PLOGI << "Skipping already initialized renderer: tpd::SurfaceRenderer";
        return;
    }
    PLOGI << "Initializing renderer: HeadlessRenderer";
    _framebufferSize = vk::Extent2D{ frameWidth, frameHeight };
}
