#pragma once

#include "torpedo/rendering/Renderer.h"

namespace tpd {
    class HeadlessRenderer final : public Renderer {
    public:
        [[nodiscard]] vk::Extent2D getFramebufferSize() const noexcept override;
        [[nodiscard]] uint32_t getInFlightFrameCount() const noexcept override;

        [[nodiscard]] uint32_t getCurrentFrameIndex() const noexcept override;
        [[nodiscard]] FrameSync getCurrentFrameSync() const noexcept override;

        static constexpr uint32_t IN_FLIGHT_FRAME_COUNT{ 1 };

    private:
        HeadlessRenderer() = default;
        void init(uint32_t frameWidth, uint32_t frameHeight) override;

        bool initialized() const noexcept override;
        void engineInit(uint32_t graphicsFamily, uint32_t transferFamily, std::pmr::memory_resource* frameResource) override;

        vk::Extent2D _framebufferSize{};
        std::array<FrameSync, IN_FLIGHT_FRAME_COUNT> _frameSyncs{};
        uint32_t _currentFrame{ 0 };

        template<RendererImpl R>
        friend class Context;
    };
} // namespace tpd

inline bool tpd::HeadlessRenderer::initialized() const noexcept {
    return _framebufferSize.width > 0 && _framebufferSize.height > 0;
}

inline vk::Extent2D tpd::HeadlessRenderer::getFramebufferSize() const noexcept {
    return _framebufferSize;
}

inline uint32_t tpd::HeadlessRenderer::getInFlightFrameCount() const noexcept {
    return 1;
}

inline uint32_t tpd::HeadlessRenderer::getCurrentFrameIndex() const noexcept {
    return _currentFrame;
}
inline tpd::Renderer::FrameSync tpd::HeadlessRenderer::getCurrentFrameSync() const noexcept {
    return _frameSyncs[_currentFrame];
}
