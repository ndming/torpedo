#include "torpedo/graphics/Drawable.h"

#include "torpedo/graphics/Material.h"
#include "torpedo/renderer/StandardRenderer.h"

std::shared_ptr<tpd::Drawable> tpd::Drawable::Builder::build(
    const std::shared_ptr<Geometry>& geometry,
    const std::shared_ptr<MaterialInstance>& materialInstance) const
{
    const auto indexCount = _indexCount == 0 ? geometry->getIndexCount() : _indexCount;
    return std::make_shared<Drawable>(geometry, materialInstance, indexCount, _instanceCount, _firstIndex, _vertexOffset, _firstInstance);
}

void tpd::Drawable::record(const vk::CommandBuffer buffer, const uint32_t frameIndex) const {
    Composable::record(buffer, frameIndex);

    // Set all dynamic states specific for this Geometry and MaterialInstance
    buffer.setPrimitiveTopology(_geometry->getPrimitiveTopology());
    buffer.setCullMode(_materialInstance->cullMode);
    StandardRenderer::vkCmdSetPolygonMode(buffer, static_cast<VkPolygonMode>(_materialInstance->polygonMode));
    buffer.setLineWidth(_materialInstance->lineWidth);

    const auto& bindings   = _geometry->getBindingDescriptions();
    const auto& attributes = _geometry->getAttributeDescriptions();
    StandardRenderer::vkCmdSetVertexInput(buffer,
        static_cast<uint32_t>(bindings.size()), bindings.data(),
        static_cast<uint32_t>(attributes.size()), attributes.data());

    // Bind vertex and index buffers
    const auto& vertexBuffer = _geometry->getVertexBuffer();
    const auto& indexBuffer  = _geometry->getIndexBuffer();
    const auto vertexBuffers = std::vector(vertexBuffer->getBufferCount(), vertexBuffer->getBuffer());
    buffer.bindVertexBuffers(0, vertexBuffers, vertexBuffer->getOffsets());
    buffer.bindIndexBuffer(indexBuffer->getBuffer(), 0, _geometry->getIndexType());

    // Bind descriptor sets
    _materialInstance->activate(buffer, frameIndex);

    // Draw this drawable
    buffer.drawIndexed(_indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}
