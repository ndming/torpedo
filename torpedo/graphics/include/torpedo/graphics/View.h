#pragma once

#include "torpedo/graphics/Scene.h"

namespace tpd {
    class View final {
    public:
        View(const vk::Viewport& viewport, vk::Rect2D scissor);

        View(const View&) = delete;
        View& operator=(const View&) = delete;

        void setSize(uint32_t width, uint32_t height);

        void setViewport(float width, float height, float offsetX = 0.0f, float offsetY = 0.0f);
        vk::Viewport viewport;

        void setScissor(uint32_t extentX, uint32_t extentY, int32_t offsetX = 0, int32_t offsetY = 0);
        vk::Rect2D scissor;

        const std::unique_ptr<Scene> scene{ std::make_unique<Scene>() };
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::View::View(const vk::Viewport& viewport, const vk::Rect2D scissor)
    : viewport{ viewport }, scissor{ scissor } {
}

inline void tpd::View::setViewport(const float width, const float height, const float offsetX, const float offsetY) {
    viewport = vk::Viewport{ offsetX, offsetY, width, height, 0.0f, 1.0f };
}

inline void tpd::View::setScissor(const uint32_t extentX, const uint32_t extentY, const int32_t offsetX, const int32_t offsetY) {
    scissor = vk::Rect2D{ vk::Offset2D{ offsetX, offsetY }, vk::Extent2D{ extentX, extentY } };
}
