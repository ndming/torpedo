#include "torpedo/graphics/Scene.h"

#include <plog/Log.h>

void tpd::Scene::insert(const std::shared_ptr<Drawable>& drawable) {
    _drawableGraph[drawable->getMaterialInstance()->getMaterial()].insert(drawable);
}

void tpd::Scene::remove(const std::shared_ptr<Drawable>& drawable) {
    _drawableGraph[drawable->getMaterialInstance()->getMaterial()].erase(drawable);
}

void tpd::Scene::insert(const std::shared_ptr<DirectionalLight>& light) {
    if (_directionalLights.size() >= Light::MAX_DIRECTIONAL_LIGHTS) {
        PLOGW << "Scene - Could not add DirectionalLight: maximum number of directional light has been reached";
    } else {
        _directionalLights.insert(light);
    }
}

void tpd::Scene::remove(const std::shared_ptr<DirectionalLight>& light) {
    _directionalLights.erase(light);
}

void tpd::Scene::insert(const std::shared_ptr<PointLight> &light) {
    if (_pointLights.size() >= Light::MAX_POINT_LIGHTS) {
        PLOGW << "Scene - Could not add PointLight: maximum number of point light has been reached";
    } else {
        _pointLights.insert(light);
    }
}

void tpd::Scene::remove(const std::shared_ptr<PointLight>& light) {
    _pointLights.erase(light);
}
