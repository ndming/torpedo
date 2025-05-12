#pragma once

#include <torpedo/math/vec4.h>

#include <array>
#include <vector>

namespace tpd {
    struct GaussianPoint {
        // Maximum number of floats for RGB spherical harmonics
        static constexpr uint32_t MAX_SH_FLOATS = 48;

        [[nodiscard]] static std::vector<GaussianPoint> random(
            uint32_t count,
            float radius = 1.0f,
            const vec3& center = { 0.f, 0.f, 0.f },
            float minScale = 0.1f,
            float maxScale = 1.0f,
            float minOpacity = 0.1f,
            float maxOpacity = 1.0f);

        vec3 position;
        float opacity;
        vec4 quaternion;
        vec4 scale;
        std::array<float, MAX_SH_FLOATS> sh;
    };

    struct GaussianCloud {
        const std::byte* data;
        uint32_t pointCount;
    };
}