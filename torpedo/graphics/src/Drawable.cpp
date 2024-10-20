#include "torpedo/graphics/Drawable.h"
#include "torpedo/graphics/Material.h"

std::shared_ptr<tpd::Drawable> tpd::Drawable::Builder::build(
    const std::shared_ptr<Geometry>& geometry,
    const std::shared_ptr<MaterialInstance>& materialInstance) const
{
    const auto indexCount = _indexCount == 0 ? geometry->getIndexCount() : _indexCount;
    return std::make_shared<Drawable>(geometry, materialInstance, indexCount, _instanceCount, _firstIndex, _vertexOffset, _firstInstance);
}
