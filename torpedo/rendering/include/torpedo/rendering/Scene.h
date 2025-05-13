#pragma once

#include <entt/entity/registry.hpp>

namespace tpd {
    using Entity = entt::entity;

    class Scene final {
    public:
        template<typename T, typename... Args> requires (std::constructible_from<T, Args...>)
        [[nodiscard]] Entity add(Args&&... args);

        template<typename T>
        [[nodiscard]] uint32_t countElement() const noexcept;

        template<typename T>
        [[nodiscard]] uint32_t countCitizen() const noexcept;

    private:
        entt::registry _registry;
    };
} // namespace tpd

template<typename T, typename... Args> requires (std::constructible_from<T, Args...>)
tpd::Entity tpd::Scene::add(Args&&... args) {
    const auto entity = _registry.create();
    _registry.emplace<T>(entity, std::forward<Args>(args)...);
    return entity;
}

template<typename T>
uint32_t tpd::Scene::countElement() const noexcept {
    return 0;
}

template<typename T>
uint32_t tpd::Scene::countCitizen() const noexcept {
    return 0;
}
