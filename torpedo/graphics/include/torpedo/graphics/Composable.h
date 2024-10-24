#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include <unordered_set>

namespace tpd {
    class Composable : public std::enable_shared_from_this<Composable> {
    public:
        Composable(const Composable&) = delete;
        Composable& operator=(const Composable&) = delete;

        void attach(const std::shared_ptr<Composable>& child);
        void detach(const std::shared_ptr<Composable>& child) noexcept;

        void attachAll(std::initializer_list<std::shared_ptr<Composable>> children);
        void detachAll() noexcept;

        [[nodiscard]] bool attached() const noexcept;
        [[nodiscard]] bool hasChild(const std::shared_ptr<Composable>& child) const noexcept;

        const std::unordered_set<std::shared_ptr<Composable>>& getChildren() const noexcept;

        void setTransform(const glm::mat4& transform);
        [[nodiscard]] const glm::mat4& getTransform() const;
        [[nodiscard]] const glm::mat4& getTransformWorld() const;

        virtual void record(vk::CommandBuffer buffer, uint32_t frameIndex) const;

        virtual ~Composable();

    protected:
        Composable() = default;

    private:
        std::weak_ptr<Composable> _parent{};
        std::unordered_set<std::shared_ptr<Composable>> _children{};

        void updateWorldTransform();
        glm::mat4 _transform{ 1.0f };
        glm::mat4 _transformWorld{ 1.0f };
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline bool tpd::Composable::attached() const noexcept {
    return !_parent.expired();
}

inline bool tpd::Composable::hasChild(const std::shared_ptr<Composable>& child) const noexcept {
    return _children.contains(child);
}

inline const std::unordered_set<std::shared_ptr<tpd::Composable>>& tpd::Composable::getChildren() const noexcept {
    return _children;
}

inline const glm::mat4& tpd::Composable::getTransform() const {
    return _transform;
}

inline const glm::mat4& tpd::Composable::getTransformWorld() const {
    return _transformWorld;
}

inline void tpd::Composable::record(const vk::CommandBuffer buffer, const uint32_t frameIndex) const {
}