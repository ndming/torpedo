#include "torpedo/graphics/Scene.h"

#include <plog/Log.h>

void tpd::Scene::insert(const std::shared_ptr<Drawable>& drawable) {
    for (const auto& mesh : drawable->getMeshes()) {
        _meshGraph[mesh.materialInstance->getMaterial()].insert(&mesh);
    }
    for (const auto& child : drawable->getChildren()) {
        if (const auto childDrawable = std::dynamic_pointer_cast<Drawable>(child)) {
            insert(childDrawable);
        }
    }
}

void tpd::Scene::remove(const std::shared_ptr<Drawable>& drawable) {
    for (auto& [_, meshes] : _meshGraph) {
        for (const auto& mesh : drawable->getMeshes()) {
            meshes.erase(&mesh);
        }
    }
    for (const auto& child : drawable->getChildren()) {
        if (const auto childDrawable = std::dynamic_pointer_cast<Drawable>(child)) {
            remove(childDrawable);
        }
    }
}

void tpd::Scene::insert(const std::shared_ptr<AmbientLight>& light) {
    if (_ambientLights.size() >= Light::MAX_AMBIENT_LIGHTS) {
        PLOGW << "Scene - Could not add AmbientLight: maximum number of ambient light has been reached";
    } else {
        _ambientLights.insert(light);
    }
}

void tpd::Scene::remove(const std::shared_ptr<AmbientLight>& light) {
    _ambientLights.erase(light);
}

void tpd::Scene::insert(const std::shared_ptr<DistantLight>& light) {
    if (_distantLights.size() >= Light::MAX_DISTANT_LIGHTS) {
        PLOGW << "Scene - Could not add DistantLight: maximum number of distant light has been reached";
    } else {
        _distantLights.insert(light);
    }
}

void tpd::Scene::remove(const std::shared_ptr<DistantLight>& light) {
    _distantLights.erase(light);
}

void tpd::Scene::insert(const std::shared_ptr<PointLight>& light) {
    if (_pointLights.size() >= Light::MAX_POINT_LIGHTS) {
        PLOGW << "Scene - Could not add PointLight: maximum number of point light has been reached";
    } else {
        _pointLights.insert(light);
    }
}

void tpd::Scene::remove(const std::shared_ptr<PointLight>& light) {
    _pointLights.erase(light);
}
