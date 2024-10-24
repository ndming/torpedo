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
            std::array<float, 3> _position{ 0.0f, 0.0f, 0.0f };
            float _decay{ 2.0f };
        };

        PointLight(std::array<float, 3> position, float decay, std::array<float, 3> color, float intensity);

        PointLight(const PointLight&) = delete;
        PointLight& operator=(const PointLight&) = delete;

        void setPosition(float x, float y, float z);
        std::array<float, 3> getPosition() const;

        float decay;

    private:
        glm::vec3 _position;
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::PointLight::Builder& tpd::PointLight::Builder::position(const float x, const float y, const float z) {
    _position[0] = x;
    _position[1] = y;
    _position[2] = z;
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
    , decay{ decay }
    , _position{ position[0], position[1], position[2] } {
}

inline std::array<float, 3> tpd::PointLight::getPosition() const {
    return { _position.x, _position.y, _position.z };
}
