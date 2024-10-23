#pragma once

#include "torpedo/graphics/Control.h"

#include <plog/Log.h>

namespace tpd {
    class OrbitControl final : public Control {
    public:
        class Builder {
        public:
            Builder& dampingFactor(float factor);
            Builder& sensitivity(float sensitivity);
            Builder& cameraUp(float x, float y, float z);
            Builder& initialTheta(float degrees);
            Builder& initialPhi(float degrees);
            Builder& initialRadius(float radius);

            [[nodiscard]] std::unique_ptr<OrbitControl> build(const Context& context) const;

        private:
            float _dampingFactor{ 0.01f };
            float _sensitivity{ 0.5f };
            std::array<float, 3> _up{ 0.0f, 0.0f, 1.0f };
            float _initialTheta{ 0.785f };
            float _initialPhi{ 0.9f };
            float _initialRadius{ 8.0f };
        };

        OrbitControl(const Context& context, float initialTheta, float initialPhi, float initialRadius);

        OrbitControl(const OrbitControl&) = delete;
        OrbitControl& operator=(const OrbitControl&) = delete;

        void update(const View& view, float deltaTimeMillis) override;

        void setDampingFactor(float factor);
        void setSensitivity(float sensitivity);
        void setCameraUp(float x, float y, float z);

    private:
        void updateVelocities(bool inViewport, float deltaTimeMillis);
        void updateCameraPosition(const std::shared_ptr<Camera>& camera);
        void updateZoomDecay(float deltaTimeMillis);

        float _dampingFactor{ 0.01f };
        float _sensitivity{ 0.5f };
        glm::vec3 _cameraUp{ 0.0f, 0.0f, 1.0f };

        float _theta;
        float _phi;
        float _radius;

        glm::vec2 _panVelocity{ 0.0f, 0.0f };
        glm::vec2 _tarVelocity{ 0.0f, 0.0f };
        float _zoomVelocity{ 0.0f };

        static constexpr auto ZOOM_FACTOR = 0.0025f;
        static constexpr auto ZOOM_DECAY = 0.99f;
        static constexpr auto PAN_FACTOR = 0.01f;

    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::OrbitControl::Builder& tpd::OrbitControl::Builder::dampingFactor(const float factor) {
    _dampingFactor = factor;
    return *this;
}

inline tpd::OrbitControl::Builder& tpd::OrbitControl::Builder::sensitivity(const float sensitivity) {
    _sensitivity = sensitivity;
    return *this;
}

inline tpd::OrbitControl::Builder& tpd::OrbitControl::Builder::cameraUp(const float x, const float y, const float z) {
    _up[0] = x;
    _up[1] = y;
    _up[2] = z;
    return *this;
}

inline tpd::OrbitControl::Builder& tpd::OrbitControl::Builder::initialTheta(const float degrees) {
    _initialTheta = glm::radians(degrees);
    return *this;
}

inline tpd::OrbitControl::Builder& tpd::OrbitControl::Builder::initialPhi(const float degrees) {
    _initialPhi = glm::radians(degrees);
    return *this;
}

inline tpd::OrbitControl::Builder& tpd::OrbitControl::Builder::initialRadius(const float radius) {
    _initialRadius = radius;
    return *this;
}

inline tpd::OrbitControl::OrbitControl(const Context& context, const float initialTheta, const float initialPhi, const float initialRadius)
    : Control{ context }, _theta{ initialTheta }, _phi{ initialPhi }, _radius { initialRadius } {
}

inline void tpd::OrbitControl::setDampingFactor(const float factor) {
    if (factor < 0.0f || factor > 1.0f) {
        PLOGW << "OrbitControl - Clamping damping factor to [0.0, 1.0]";
    }
    _dampingFactor = std::clamp(factor, 0.0f, 1.0f);
}

inline void tpd::OrbitControl::setSensitivity(const float sensitivity) {
    if (sensitivity < 0.0f) {
        PLOGW << "OrbitControl - Sensitivity must be non-negative, re-assigning to 0.0";
    }
    _sensitivity = std::max(sensitivity, 0.0f);
}

inline void tpd::OrbitControl::setCameraUp(const float x, const float y, const float z) {
    _cameraUp = normalize(glm::vec3{ x, y, z });
}
