#include "torpedo/extension/PerspectiveCamera.h"

void tpd::PerspectiveCamera::setVerticalFov(const float degrees) noexcept {
    const auto fovY = degrees * std::numbers::pi_v<float> / 180.f;
    const auto fy = 1.f / std::tan(fovY * 0.5f);
    updateProjectionMatrix(fy);
}

void tpd::PerspectiveCamera::updateProjectionMatrix(const float fy) noexcept {
    const auto fx = fy / _aspect;

    // Map view depth according to reversed z-mapping, i.e. [near, far] are mapped to [1, 0]
    // See: https://developer.nvidia.com/content/depth-precision-visualized
    const auto za = _near / (_near - _far);
    const auto zb = _near * _far / (_far - _near);

    _projection = {
        fx,  0.f, 0.f, 0.f,
        0.f, fy,  0.f, 0.f,
        0.f, 0.f, za,  zb,
        0.f, 0.f, 1.f, 0.f,
    };
}
