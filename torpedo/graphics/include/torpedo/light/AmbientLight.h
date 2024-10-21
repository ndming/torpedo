#pragma once

#include "torpedo/graphics/Light.h"

namespace tpd {
    class AmbientLight final : public Light {
    public:
        class Builder final : public Light::Builder<Builder, AmbientLight> {
        public:
            [[nodiscard]] std::shared_ptr<AmbientLight> build() const override;
        };

        AmbientLight(std::array<float, 3> color, float intensity);

        AmbientLight(const AmbientLight&) = delete;
        AmbientLight& operator=(const AmbientLight&) = delete;
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::AmbientLight::AmbientLight(const std::array<float, 3> color, const float intensity) : Light{ color, intensity } {
}
