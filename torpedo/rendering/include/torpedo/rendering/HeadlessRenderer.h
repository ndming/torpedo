#pragma once

#include "torpedo/rendering/Renderer.h"

namespace tpd {
    class HeadlessRenderer final : public Renderer {
    public:
        HeadlessRenderer(const HeadlessRenderer&) = delete;
        HeadlessRenderer& operator=(const HeadlessRenderer&) = delete;

        [[nodiscard]] vk::Extent2D getFramebufferSize() const noexcept override;
        [[nodiscard]] uint32_t getInFlightFramesCount() const noexcept override;

    private:
        HeadlessRenderer() = default;
        void init(uint32_t frameWidth, uint32_t frameHeight, std::pmr::memory_resource* contextPool) override;
        vk::Extent2D _framebufferSize{};

        void engineInit(uint32_t graphicsFamilyIndex, uint32_t presentFamilyIndex) override;

        template<RendererImpl R>
        friend class Context;
    };
}

// =========================== //
// INLINE FUNCTION DEFINITIONS //
// =========================== //

inline vk::Extent2D tpd::HeadlessRenderer::getFramebufferSize() const noexcept {
    return _framebufferSize;
}

inline uint32_t tpd::HeadlessRenderer::getInFlightFramesCount() const noexcept {
    return 1;
}
