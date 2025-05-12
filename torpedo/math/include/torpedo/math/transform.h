#pragma once

#include "torpedo/math/vec3.h"

namespace tpd::math {
    [[nodiscard]] vec3 to_cartesian(float theta, float phi, float radius = 1.0f) noexcept;
    [[nodiscard]] vec3 to_spherical(float x, float y, float z) noexcept;
}

inline tpd::vec3 tpd::math::to_cartesian(const float theta, const float phi, const float radius) noexcept {
    const float x = radius * std::sin(phi) * std::cos(theta);
    const float y = radius * std::sin(phi) * std::sin(theta);
    const float z = radius * std::cos(phi);

    return { x, y, z };
}

inline tpd::vec3 tpd::math::to_spherical(const float x, const float y, const float z) noexcept {
    const float radius = std::sqrt(x * x + y * y + z * z);
    const float theta = std::atan2(y, x);
    const float phi = std::acos(z / radius);

    return { theta, phi, radius };
}
