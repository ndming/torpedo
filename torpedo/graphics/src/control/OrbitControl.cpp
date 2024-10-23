#include "torpedo/control/OrbitControl.h"

#include <numbers>

std::unique_ptr<tpd::OrbitControl> tpd::OrbitControl::Builder::build(const Context& context) const {
    auto orbitControl = std::make_unique<OrbitControl>(context, _initialTheta, _initialPhi, _initialRadius);
    orbitControl->setDampingFactor(_dampingFactor);
    orbitControl->setSensitivity(_sensitivity);
    orbitControl->setCameraUp(_up[0], _up[1], _up[2]);
    return orbitControl;
}

void tpd::OrbitControl::update(const View& view, const float deltaTimeMillis) {
    updateDeltaMouse();
    updateVelocities(mousePositionIn(view.viewport), deltaTimeMillis);
    updateCameraPosition(view.camera);
    updateZoomDecay(deltaTimeMillis);
}

void tpd::OrbitControl::updateVelocities(const bool inViewport, const float deltaTimeMillis) {
    if (inViewport && _mouseLeftDragging) {
        // Capture pan velocity in pixels/s
        _panVelocity = _deltaMouse / deltaTimeMillis;
    } else if (!_mouseLeftDragging) {
        // Apply exponential decay to the pan velocity
        _panVelocity *= glm::pow(1.0f - _dampingFactor, deltaTimeMillis);
    }

    // Do the same for target velocity
    if (inViewport && _mouseMiddleDragging) {
        _tarVelocity = _deltaMouse / deltaTimeMillis;
    } else if (!_mouseMiddleDragging) {
        _tarVelocity *= glm::pow(1.0f - _dampingFactor, deltaTimeMillis);
    }

    // Don't update zoom if not scrolling
    if (_deltaScroll.y != 0.0f) {
        _zoomVelocity = _deltaScroll.y * deltaTimeMillis;
    }
}

void tpd::OrbitControl::updateCameraPosition(const std::shared_ptr<Camera>& camera) {
    _theta -= _panVelocity.x * _sensitivity * PAN_FACTOR;
    _phi   -= _panVelocity.y * _sensitivity * PAN_FACTOR;
    _phi = std::clamp(_phi, 0.0f + 0.01f, std::numbers::pi_v<float> - 0.01f);

    // Zoom should be proportional to the current radius and must not proportional to sensitivity
    _radius -= _radius * _zoomVelocity * ZOOM_FACTOR;

    const float camX = _radius * std::sin(_phi) * std::cos(_theta);
    const float camY = _radius * std::sin(_phi) * std::sin(_theta);
    const float camZ = _radius * std::cos(_phi);

    camera->lookAt({ camX, camY, camZ }, glm::vec3{ 0.0f }, _cameraUp);
}

void tpd::OrbitControl::updateZoomDecay(const float deltaTimeMillis) {
    // Zoom decay factor is tailored to a sweet value, we don't allow outsiders to set this
    _zoomVelocity *= glm::pow(0.99f, deltaTimeMillis);
    // Set delta scroll to 0 to prevent zoom velocity stuck on updating the old value
    _deltaScroll.y = 0.0f;
}
