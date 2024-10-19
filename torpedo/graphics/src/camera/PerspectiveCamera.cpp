#include "torpedo/camera/PerspectiveCamera.h"

tpd::PerspectiveCamera::PerspectiveCamera(const uint32_t filmWidth, const uint32_t filmHeight) : Camera{ filmWidth, filmHeight } {
    const auto ratio = static_cast<float>(filmWidth) / static_cast<float>(filmHeight);
    _proj = glm::perspective(glm::radians(45.0f), ratio, 0.1f, 32.0f);
    _proj[1][1] *= -1.0f;
}

void tpd::PerspectiveCamera::setProjection(const float fov, const float aspect, const float near, const float far) {
    _proj = glm::perspective(glm::radians(fov), aspect, near, far);
    _proj[1][1] *= -1.0f;
}
