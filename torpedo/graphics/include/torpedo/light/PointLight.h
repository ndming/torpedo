#pragma once

#include "torpedo/graphics/Light.h"
#include "torpedo/graphics/Composable.h"

namespace tpd {
    class PointLight final : public Light, public Composable {
    public:
        class Builder final : public Light::Builder<Builder, PointLight> {
        public:
            Builder& position(float x, float y, float z);
            Builder& decay(float rate);

            [[nodiscard]] std::shared_ptr<PointLight> build() const override;

        private:
            float _posX{ 0.0f };
            float _posY{ 0.0f };
            float _posZ{ 0.0f };
            float _decay{ 2.0f };
        };

        PointLight(std::array<float, 3> position, float decay, std::array<float, 3> color, float intensity);

        void setPosition(float x, float y, float z);

        std::array<float, 3> position;
        float decay;
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::PointLight::Builder& tpd::PointLight::Builder::position(const float x, const float y, const float z) {
    _posX = x;
    _posY = y;
    _posZ = z;
    return *this;
}

inline tpd::PointLight::Builder& tpd::PointLight::Builder::decay(const float rate) {
    _decay = rate;
    return *this;
}

inline tpd::PointLight::PointLight(
    const std::array<float, 3> position,
    const float decay,
    const std::array<float, 3> color,
    const float intensity)
    : Light{ color, intensity }
    , position{ position }
    , decay{ decay } {
}

inline void tpd::PointLight::setPosition(const float x, const float y, const float z) {
    position[0] = x;
    position[1] = y;
    position[2] = z;
}
