#include "torpedo/extension/OrbitControl.h"

#include <numbers>

std::pair<tpd::vec3, tpd::vec3> tpd::OrbitControl::getCameraUpdate(const float deltaTimeMillis) noexcept {
    updateDeltaMouse();

    updateVelocities(deltaTimeMillis);
    decayZoom(deltaTimeMillis);

    updateCameraPosition();
    updateCameraTarget();

    return { math::to_cartesian(_theta, _phi, _radius) + _target, _target };
}

void tpd::OrbitControl::updateVelocities(const float deltaTimeMillis) {
    _panVelocity = _mouseLeftDragging
        ? _deltaMousePosition * deltaTimeMillis
        : vec2{ 0.f, 0.f };

    _tarVelocity = _mouseRightDragging
        ? _deltaMousePosition * deltaTimeMillis
        : vec2{ 0.f, 0.f };

    // Don't update zoom if not scrolling
    if (_deltaScroll.y != 0.0f) {
        _zoomVelocity = _deltaScroll.y * deltaTimeMillis;
    }
}

void tpd::OrbitControl::decayZoom(const float deltaTimeMillis) {
    // Zoom decay factor is tailored to a sweet value, we don't allow outsiders to set this
    _zoomVelocity *= std::pow(0.99f, deltaTimeMillis);
    // Set delta scroll to 0 to prevent zoom velocity stuck on updating the old value
    _deltaScroll.y = 0.0f;
}

void tpd::OrbitControl::updateCameraPosition() noexcept {
    _theta -= _panVelocity.x * _sensitivity * PAN_FACTOR;
    _phi   -= _panVelocity.y * _sensitivity * PAN_FACTOR;
    _phi = std::clamp(_phi, 0.0f + 0.01f, std::numbers::pi_v<float> - 0.01f);

    // Zoom should be proportional to the current radius and must not proportional to sensitivity
    _radius -= _radius * _zoomVelocity * ZOOM_FACTOR;
    _radius = std::max(_radius, 0.1f);
}

void tpd::OrbitControl::updateCameraTarget() noexcept {
    using namespace math;
    const auto forward = _target - to_cartesian(_theta, _phi, _radius);
    const auto dirX = normalize(cross(forward, getCameraUp()));
    const auto dirY = normalize(cross(forward, dirX));
    _target -= dirX * _tarVelocity.x * _sensitivity * PAN_FACTOR * 4.f;
    _target -= dirY * _tarVelocity.y * _sensitivity * PAN_FACTOR * 4.f;
}
