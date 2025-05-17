#include "torpedo/rendering/TransformHost.h"

void tpd::TransformHost::transform(const Entity entity, const mat4& transform) const {
    if (!_entityMap.contains(entity)) [[unlikely]] {
        return;
    }

    const auto offset = sizeof(mat4) * _entityMap.at(entity);
    const auto bufferIndex = _renderer ? _renderer->getCurrentFrameIndex() : 0;
    _transformBuffer->update(bufferIndex, &transform, sizeof(mat4), offset);
    vmaFlushAllocation(_allocator, _transformBuffer->getAllocation(), offset, sizeof(mat4));
}