#pragma once

#include <torpedo/rendering/Control.h>
#include <torpedo/rendering/Camera.h>

#include <torpedo/math/vec3.h>
#include <torpedo/math/transform.h>

namespace tpd {
    class OrbitControl final : public Control {
    public:
        explicit OrbitControl(GLFWwindow* window) : Control(window) {}

        void setSensitivity(float sensitivity) noexcept;
        void setEyePosition(float x, float y, float z) noexcept;

        void setRadius(float radius) noexcept;
        void setTarget(const vec3& target) noexcept;

        [[nodiscard]] std::pair<vec3, vec3> getCameraUpdate(float deltaTimeMillis) noexcept;
        [[nodiscard]] static constexpr vec3 getCameraUp() noexcept { return { 0.f, 0.f, 1.f }; }

    private:
        void updateVelocities(float deltaTimeMillis);
        void decayZoom(float deltaTimeMillis);

        void updateCameraPosition() noexcept;
        void updateCameraTarget() noexcept;

        float _sensitivity{ 1.f };

        float _theta{ .785f };
        float _phi{ .9f };
        float _radius{ 1.0f };
        vec3 _target{ 0.f, 0.f, 0.f };

        vec2 _panVelocity{ 0.f, 0.f };
        vec2 _tarVelocity{ 0.f, 0.f };
        float _zoomVelocity{ 0.f };

        static constexpr auto ZOOM_FACTOR = 0.0025f;
        static constexpr auto PAN_FACTOR = 0.00075f;
    };
} // namespace tpd

inline void tpd::OrbitControl::setSensitivity(const float sensitivity) noexcept {
    _sensitivity = sensitivity;
}

inline void tpd::OrbitControl::setEyePosition(const float x, const float y, const float z) noexcept {
    const auto vec = math::to_spherical(x, y, z);
    _theta = vec.x;
    _phi = vec.y;
    _radius = vec.z;
}

inline void tpd::OrbitControl::setRadius(const float radius) noexcept {
    _radius = radius;
}

inline void tpd::OrbitControl::setTarget(const vec3& target) noexcept {
    _target = target;
}