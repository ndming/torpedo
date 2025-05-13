#pragma once

#include <entt/entity/registry.hpp>

#include <execution>
#include <map>
#include <numeric>
#include <ranges>
#include <span>

namespace tpd {
    using Entity = entt::entity;

    template<typename T> requires (!std::is_rvalue_reference_v<T>)
    using EntityGroup = std::span<const T>;

    template<typename T>
    [[nodiscard]] EntityGroup<T> group(const std::vector<T>& elements) {
        return EntityGroup{ elements.data(), elements.size() };
    }

    class Scene final {
    public:
        template<typename T, typename... Args> requires (std::constructible_from<T, Args...>)
        Entity add(Args&&... args);

        template<typename T>
        Entity add(T&& element);

        template<typename T>
        Entity add(EntityGroup<T> elements);

        template<typename T>
        [[nodiscard]] uint32_t count() const noexcept;

        template<typename T>
        [[nodiscard]] uint32_t countGroup() const noexcept;

        template<typename T>
        [[nodiscard]] uint32_t countAll() const noexcept;

        template<typename T>
        [[nodiscard]] std::vector<std::byte> data() const;

        template<typename T>
        [[nodiscard]] std::vector<std::byte> dataGroup() const;

        template<typename T>
        [[nodiscard]] std::vector<std::byte> dataAll() const;

        template<typename T>
        [[nodiscard]] std::vector<uint32_t> groupSizes() const noexcept;

        template<typename T>
        [[nodiscard]] std::map<Entity, uint32_t> buildEntityMap() const;

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
tpd::Entity tpd::Scene::add(T&& element) {
    const auto entity = _registry.create();
    _registry.emplace<T>(entity, std::forward<T>(element));
    return entity;
}

template<typename T>
tpd::Entity tpd::Scene::add(const EntityGroup<T> elements) {
    const auto entity = _registry.create();
    _registry.emplace<std::span<const T>>(entity, elements);
    return entity;
}

template<typename T>
uint32_t tpd::Scene::count() const noexcept {
    return _registry.view<T>().size();
}

template<typename T>
uint32_t tpd::Scene::countGroup() const noexcept {
    return _registry.view<EntityGroup<T>>().size();
}

template<typename T>
uint32_t tpd::Scene::countAll() const noexcept {
    const auto groups = _registry.view<EntityGroup<T>>();
    return std::transform_reduce(
        std::execution::par, groups.begin(), groups.end(), count<T>(), std::plus{},
        [this](const auto entity) { return static_cast<uint32_t>(_registry.get<EntityGroup<T>>(entity).size()); });
}

template<typename T>
std::vector<std::byte> tpd::Scene::data() const {
    const auto toType = [this](const auto entity) { return _registry.get<T>(entity); };
    const auto types = _registry.view<T>() | std::views::transform(toType) | std::ranges::to<std::vector>();
    const auto size = types.size() * sizeof(T);
    auto bytes = std::vector<std::byte>(size);
    std::memcpy(bytes.data(), types.data(), size);
    return bytes;
}

template<typename T>
std::vector<std::byte> tpd::Scene::dataGroup() const {
    auto data = std::vector<std::byte>((countAll<T>() - count<T>()) * sizeof(T));

    auto offset = 0;
    const auto groups = _registry.view<EntityGroup<T>>();
    for (const auto entity : groups) {
        const auto group = _registry.get<EntityGroup<T>>(entity);
        const auto bytes = group.size() * sizeof(T);
        std::memcpy(data.data() + offset, group.data(), bytes);
        offset += bytes;
    }

    return data;
}

template<typename T>
std::vector<std::byte> tpd::Scene::dataAll() const {
    auto data = std::vector<std::byte>(countAll<T>() * sizeof(T));

    auto offset = 0;
    const auto groups = _registry.view<EntityGroup<T>>();
    for (const auto entity : groups) {
        const auto group = _registry.get<EntityGroup<T>>(entity);
        const auto bytes = group.size() * sizeof(T);
        std::memcpy(data.data() + offset, group.data(), bytes);
        offset += bytes;
    }

    const auto bytes = this->data<T>();
    std::memcpy(data.data() + offset, bytes.data(), bytes.size());

    return data;
}

template<typename T>
std::vector<uint32_t> tpd::Scene::groupSizes() const noexcept {
    const auto toSize = [this](const auto entity) { return static_cast<uint32_t>(_registry.get<EntityGroup<T>>(entity).size()); };
    return _registry.view<EntityGroup<T>>() | std::views::transform(toSize) | std::ranges::to<std::vector>();
}

template<typename T>
std::map<tpd::Entity, uint32_t> tpd::Scene::buildEntityMap() const {
    const auto toEntity = [this](const auto entity) { return entity; };
    auto entities = std::vector<Entity>{};
    entities.reserve(count<T>() + countGroup<T>());
    std::ranges::move(_registry.view<EntityGroup<T>>() | std::views::transform(toEntity), std::back_inserter(entities));
    std::ranges::move(_registry.view<T>() | std::views::transform(toEntity), std::back_inserter(entities));
    const auto indices = std::views::iota(0u, count<T>() + countGroup<T>());
    return std::views::zip(entities, indices) | std::ranges::to<std::map>();
}