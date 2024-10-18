#pragma once

#include "torpedo/graphics/Drawable.h"

#include <unordered_map>
#include <unordered_set>

namespace tpd {
    class Scene final {
    public:
        Scene() = default;

        Scene(const Scene&) = delete;
        Scene& operator=(const Scene&) = delete;

        void insert(const std::shared_ptr<Drawable>& drawable);
        void remove(const std::shared_ptr<Drawable>& drawable);

        using DrawableGraph = std::unordered_map<const Material*, std::unordered_set<std::shared_ptr<Drawable>>>;
        [[nodiscard]] const DrawableGraph& getDrawableGraph() const;

    private:
        DrawableGraph _drawableGraph{};
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline const tpd::Scene::DrawableGraph& tpd::Scene::getDrawableGraph() const {
    return _drawableGraph;
}
