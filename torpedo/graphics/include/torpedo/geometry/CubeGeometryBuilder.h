#pragma once

#include "torpedo/graphics/Geometry.h"

namespace tpd {
    class CubeGeometryBuilder {
    public:
        CubeGeometryBuilder& halfExtent(float extent);

        [[nodiscard]] std::shared_ptr<Geometry> build(const Renderer& renderer) const;

    private:
        float _halfExtent{ 1.0f };
    };
}

inline tpd::CubeGeometryBuilder& tpd::CubeGeometryBuilder::halfExtent(const float extent) {
    _halfExtent = extent;
    return *this;
}
