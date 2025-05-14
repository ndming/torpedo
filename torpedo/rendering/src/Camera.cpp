#include "torpedo/rendering/Camera.h"

void tpd::Camera::lookAt(const vec3& eye, const vec3& center, const vec3& up) noexcept {
    using namespace math;

    const auto z = normalize(center - eye); // z forward
    const auto x = normalize(cross(z, up)); // x right
    const auto y = cross(z, x);             // y down

    _view = {
        x.x, x.y, x.z, -dot(x, eye),
        y.x, y.y, y.z, -dot(y, eye),
        z.x, z.y, z.z, -dot(z, eye),
        0.f, 0.f, 0.f, 1.f,
    };
}

void tpd::Camera::lookAt(const mat3& R, const vec3& t) noexcept {
    _view = mat4{ R, t };
}
