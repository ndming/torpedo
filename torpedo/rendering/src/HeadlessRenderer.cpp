#include "torpedo/rendering/HeadlessRenderer.h"
#include "torpedo/rendering/LogUtils.h"

void tpd::HeadlessRenderer::init(const uint32_t frameWidth, const uint32_t frameHeight) {
    if (initialized()) [[unlikely]] {
        PLOGI << "Skipping already initialized renderer: tpd::HeadlessRenderer";
        return;
    }
    PLOGI << "Initializing renderer: tpd::HeadlessRenderer";
    _framebufferSize = vk::Extent2D{ frameWidth, frameHeight };
}

void tpd::HeadlessRenderer::engineInit(
    const uint32_t graphicsFamily,
    const uint32_t transferFamily,
    std::pmr::memory_resource* frameResource)
{
}
