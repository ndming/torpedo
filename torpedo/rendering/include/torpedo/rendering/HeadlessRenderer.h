#pragma once

#include "torpedo/rendering/Renderer.h"

namespace tpd {
    class HeadlessRenderer final : public Renderer {
    public:
        HeadlessRenderer() = default;
        void init(uint32_t framebufferWidth, uint32_t framebufferHeight) override;

        HeadlessRenderer(const HeadlessRenderer&) = delete;
        HeadlessRenderer& operator=(const HeadlessRenderer&) = delete;

        [[nodiscard]] vk::Extent2D getFramebufferSize() const override;
        [[nodiscard]] uint32_t getInFlightFramesCount() const override;

    private:
        vk::Extent2D _framebufferSize{};
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline vk::Extent2D tpd::HeadlessRenderer::getFramebufferSize() const {
    return _framebufferSize;
}

inline uint32_t tpd::HeadlessRenderer::getInFlightFramesCount() const {
    return 1;
}
