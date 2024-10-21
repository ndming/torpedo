#pragma once

#include "torpedo/graphics/Light.h"

namespace tpd {
    class DistantLight final : public Light {
    public:
        class Builder final : public Light::Builder<Builder, DistantLight> {
        public:
            Builder& direction(float x, float y, float z);

            [[nodiscard]] std::shared_ptr<DistantLight> build() const override;

        private:
            float _dirX{  0.0f };
            float _dirY{  0.0f };
            float _dirZ{ -1.0f };
        };

        DistantLight(std::array<float, 3> direction, std::array<float, 3> color, float intensity);

        void setDirection(float x, float y, float z);
        std::array<float, 3> direction;
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::DistantLight::Builder& tpd::DistantLight::Builder::direction(const float x, const float y, const float z) {
    _dirX = x;
    _dirY = y;
    _dirZ = z;
    return *this;
}

inline tpd::DistantLight::DistantLight(
    const std::array<float, 3> direction,
    const std::array<float, 3> color,
    const float intensity)
    : Light{ color, intensity }
    , direction{ direction } {
}

inline void tpd::DistantLight::setDirection(const float x, const float y, const float z) {
    direction[0] = x;
    direction[1] = y;
    direction[2] = z;
}
