#pragma once

#include "torpedo/rendering/Scene.h"
#include "torpedo/rendering/Renderer.h"

#include <torpedo/foundation/RingBuffer.h>
#include <torpedo/math/mat4.h>

namespace tpd {
    class TransformHost final {
    public:
        explicit TransformHost(VmaAllocator allocator, Renderer* renderer = nullptr);

        TransformHost(const TransformHost&) = delete;
        TransformHost& operator=(const TransformHost&) = delete;

        void update(const std::map<Entity, uint32_t>& entityMap, const RingBuffer* buffer);
        void update(std::map<Entity, uint32_t>&& entityMap, const RingBuffer* buffer) noexcept;

        void transform(Entity entity, const mat4& transform) const;

    private:
        VmaAllocator _allocator;
        Renderer* _renderer;

        std::map<Entity, uint32_t> _entityMap{};
        const RingBuffer* _transformBuffer{};
    };
} // namespace tpd

inline tpd::TransformHost::TransformHost(VmaAllocator allocator, Renderer* renderer)
    : _allocator{ allocator }, _renderer { renderer } {}

inline void tpd::TransformHost::update(const std::map<Entity, uint32_t>& entityMap, const RingBuffer* buffer) {
    _entityMap = entityMap;
    _transformBuffer = buffer;
}

inline void tpd::TransformHost::update(std::map<Entity, uint32_t>&& entityMap, const RingBuffer* buffer) noexcept {
    _entityMap = std::move(entityMap);
    _transformBuffer = buffer;
}
