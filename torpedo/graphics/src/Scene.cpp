#include "torpedo/graphics/Scene.h"

void tpd::Scene::insert(const std::shared_ptr<Drawable>& drawable) {
    _drawableGraph[drawable->getMaterialInstance()->getMaterial()].insert(drawable);
}

void tpd::Scene::remove(const std::shared_ptr<Drawable>& drawable) {
    _drawableGraph[drawable->getMaterialInstance()->getMaterial()].erase(drawable);
}
