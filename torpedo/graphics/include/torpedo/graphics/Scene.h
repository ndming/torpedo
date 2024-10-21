#pragma once

#include "torpedo/graphics/Drawable.h"
#include "torpedo/light/AmbientLight.h"
#include "torpedo/light/DistantLight.h"
#include "torpedo/light/PointLight.h"

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

        void insert(const std::shared_ptr<AmbientLight>& light);
        void remove(const std::shared_ptr<AmbientLight>& light);

        void insert(const std::shared_ptr<DistantLight>& light);
        void remove(const std::shared_ptr<DistantLight>& light);

        void insert(const std::shared_ptr<PointLight>& light);
        void remove(const std::shared_ptr<PointLight>& light);

        using DrawableGraph = std::unordered_map<const Material*, std::unordered_set<std::shared_ptr<Drawable>>>;
        [[nodiscard]] const DrawableGraph& getDrawableGraph() const;

        [[nodiscard]] const std::unordered_set<std::shared_ptr<AmbientLight>>& getAmbientLights() const;
        [[nodiscard]] const std::unordered_set<std::shared_ptr<DistantLight>>& getDistantLights() const;
        [[nodiscard]] const std::unordered_set<std::shared_ptr<PointLight>>& getPointLights() const;

    private:
        DrawableGraph _drawableGraph{};

        std::unordered_set<std::shared_ptr<AmbientLight>> _ambientLights{};
        std::unordered_set<std::shared_ptr<DistantLight>> _distantLights{};
        std::unordered_set<std::shared_ptr<PointLight>> _pointLights{};
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline const tpd::Scene::DrawableGraph& tpd::Scene::getDrawableGraph() const {
    return _drawableGraph;
}

inline const std::unordered_set<std::shared_ptr<tpd::AmbientLight>>& tpd::Scene::getAmbientLights() const {
    return _ambientLights;
}

inline const std::unordered_set<std::shared_ptr<tpd::DistantLight>>& tpd::Scene::getDistantLights() const {
    return _distantLights;
}

inline const std::unordered_set<std::shared_ptr<tpd::PointLight>>& tpd::Scene::getPointLights() const {
    return _pointLights;
}
