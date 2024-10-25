#include "torpedo/graphics/Composable.h"

#include <algorithm>

void tpd::Composable::attach(const std::shared_ptr<Composable>& child) {
    // Before adding the child, we check against self attachment and multiple parents. Even with this verification,
    // it's still possible for the call site to create a composable tree containing cycles. When this happens, it
    // causes a severe recursive rendering and memory leak. We could in fact trace against the _parent member and
    // check for any circular dependency. However, this might add an extra overhead when we attach a child, causing
    // a ridiculous performance hit. Rather, given that the call site (in their right consciousness) would rarely form
    // a circular tree in practice, we will assume such a mess would never happen.
    if (child.get() != this && !child->attached()) {
        child->_parent = shared_from_this();
        child->updateWorldTransform();
        _children.insert(child);
    }
}

void tpd::Composable::detach(const std::shared_ptr<Composable>& child) noexcept {
    // Only detach when the child is a descendant of this composable
    if (hasChild(child)) {
        child->_parent.reset();
        child->updateWorldTransform();
        _children.erase(child);
    }
}

void tpd::Composable::attachAll(const std::initializer_list<std::shared_ptr<Composable>> children) {
    std::ranges::for_each(children, [this](const auto it) { attach(it); });
}

void tpd::Composable::detachAll() noexcept {
    std::ranges::for_each(_children, [this](const auto it) { detach(it); });
}

void tpd::Composable::setTransform(const glm::mat4& transform) {
    _transform = transform;
    updateWorldTransform();
}

void tpd::Composable::updateWorldTransform() {
    _transformWorld = attached() ? _parent.lock()->_transformWorld * _transform : _transform;
    std::ranges::for_each(_children, [](const auto& it) { it->updateWorldTransform(); });
}
