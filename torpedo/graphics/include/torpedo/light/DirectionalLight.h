#pragma once

#include "torpedo/graphics/Light.h"

namespace tpd {
    class DirectionalLight final : public Light {
    public:
        class Builder final : public Light::Builder<Builder, DirectionalLight> {
        public:
            Builder& direction(float x, float y, float z);

            [[nodiscard]] std::shared_ptr<DirectionalLight> build() const override;

        private:
            float _dirX{  0.0f };
            float _dirY{  0.0f };
            float _dirZ{ -1.0f };
        };

        DirectionalLight(std::array<float, 3> direction, std::array<float, 3> color, float intensity);

        void setDirection(float x, float y, float z);
        std::array<float, 3> direction;
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::DirectionalLight::Builder& tpd::DirectionalLight::Builder::direction(const float x, const float y, const float z) {
    _dirX = x;
    _dirY = y;
    _dirZ = z;
    return *this;
}

inline tpd::DirectionalLight::DirectionalLight(
    const std::array<float, 3> direction,
    const std::array<float, 3> color,
    const float intensity)
    : Light{ color, intensity }
    , direction{ direction } {
}

inline void tpd::DirectionalLight::setDirection(const float x, const float y, const float z) {
    direction[0] = x;
    direction[1] = y;
    direction[2] = z;
}
