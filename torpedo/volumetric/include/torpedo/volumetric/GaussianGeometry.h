#pragma once

#include <torpedo/math/vec4.h>

#include <array>
#include <filesystem>
#include <vector>

namespace tpd {
    struct GaussianPoint {
        // Maximum number of floats for RGB spherical harmonics
        static constexpr uint32_t MAX_SH_FLOATS = 48;

        vec3 position;
        float opacity;
        vec4 quaternion;
        vec4 scale;
        std::array<float, MAX_SH_FLOATS> sh;

        [[nodiscard]] static std::vector<GaussianPoint> random(
            uint32_t count,
            float radius = 1.0f,
            const vec3& center = { 0.f, 0.f, 0.f },
            float minScale = 0.1f,
            float maxScale = 1.0f,
            float minOpacity = 0.1f,
            float maxOpacity = 1.0f);

        [[nodiscard]] static std::vector<GaussianPoint> fromModel(const std::filesystem::path& plyFile);
    };

    namespace utils {
        [[nodiscard]] constexpr std::array<float, GaussianPoint::MAX_SH_FLOATS> rgb2sh(float r, float g, float b) noexcept;
        [[nodiscard]] constexpr vec3 sh2rgb(const std::array<float, GaussianPoint::MAX_SH_FLOATS>& sh) noexcept;
    }
} // namespace tpd

constexpr std::array<float, tpd::GaussianPoint::MAX_SH_FLOATS> tpd::utils::rgb2sh(const float r, const float g, const float b) noexcept {
    constexpr auto C0 = 0.28209479177387814f;

    auto sh = std::array<float, GaussianPoint::MAX_SH_FLOATS>{};
    sh[0] = (r - 0.5f) / C0;
    sh[1] = (g - 0.5f) / C0;
    sh[2] = (b - 0.5f) / C0;
    return sh;
}

constexpr tpd::vec3 tpd::utils::sh2rgb(const std::array<float, GaussianPoint::MAX_SH_FLOATS>& sh) noexcept {
    constexpr auto C0 = 0.28209479177387814f;
    const auto rgb = vec3{ sh[0], sh[1], sh[2] };
    return rgb * C0 + 0.5f;
}
