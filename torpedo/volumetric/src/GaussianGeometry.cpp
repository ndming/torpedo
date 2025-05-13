#include "torpedo/volumetric/GaussianGeometry.h"

#include <random>

std::vector<tpd::GaussianPoint> tpd::GaussianPoint::random(
    const uint32_t count,
    const float radius,
    const vec3& center,
    const float minScale,
    const float maxScale,
    const float minOpacity,
    const float maxOpacity)
{
    std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution dist_pos(-1.0f, 1.0f);
    std::uniform_real_distribution dist_opacity(minOpacity, maxOpacity);
    std::uniform_real_distribution dist_scale(minScale, maxScale);
    std::uniform_real_distribution dist_sh(0.0f, 1.0f);

    auto points = std::vector<GaussianPoint>(count);
    for (auto& [position, opacity, quaternion, scale, sh] : points) {
        position = vec3{ dist_pos(rng), dist_pos(rng), dist_pos(rng) } * radius + center;
        opacity = dist_opacity(rng);
        quaternion = { 0.0f, 0.0f, 0.0f, 1.0f };
        scale = { dist_scale(rng), dist_scale(rng), dist_scale(rng), 1.0f };
        sh = utils::rgb2sh(dist_sh(rng), dist_sh(rng), dist_sh(rng));
    }
    return points;
}