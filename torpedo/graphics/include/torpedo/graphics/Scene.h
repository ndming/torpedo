#pragma once

#include "torpedo/graphics/Drawable.h"

#include <unordered_map>
#include <unordered_set>

namespace tpd {
    class Scene final {
    public:
        Scene(const vk::Viewport& viewport, vk::Rect2D scissor);

        Scene(const Scene&) = delete;
        Scene& operator=(const Scene&) = delete;

        void insert(const std::shared_ptr<Drawable>& drawable);
        void remove(const std::shared_ptr<Drawable>& drawable);

        using DrawableGraph = std::unordered_map<const Material*, std::unordered_set<std::shared_ptr<Drawable>>>;
        [[nodiscard]] const DrawableGraph& getDrawableGraph() const;

        void setViewport(float x, float y, float width, float height);
        vk::Viewport viewport;

        void setScissor(int32_t offsetX, int32_t offsetY, uint32_t extentX, uint32_t extentY);
        vk::Rect2D scissor;

    private:
        DrawableGraph _drawableGraph{};
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::Scene::Scene(const vk::Viewport& viewport, const vk::Rect2D scissor)
    : viewport{ viewport }, scissor{ scissor } {
}

inline const tpd::Scene::DrawableGraph& tpd::Scene::getDrawableGraph() const {
    return _drawableGraph;
}

inline void tpd::Scene::setViewport(const float x, const float y, const float width, const float height) {
    viewport = vk::Viewport{ x, y, width, height };
}

inline void tpd::Scene::setScissor(const int32_t offsetX, const int32_t offsetY, const uint32_t extentX, const uint32_t extentY) {
    scissor = vk::Rect2D{ vk::Offset2D{ offsetX, offsetY }, vk::Extent2D{ extentX, extentY } };
}
