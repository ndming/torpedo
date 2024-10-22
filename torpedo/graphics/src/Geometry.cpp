#include "torpedo/graphics/Geometry.h"
#include "torpedo/graphics/Renderer.h"

#include <plog/Log.h>

tpd::Geometry::Builder& tpd::Geometry::Builder::attributeCount(const uint32_t count) {
    _bindings.reserve(count);
    _attributes.reserve(count);
    _attributeBindings.reserve(count);
    return *this;
}

tpd::Geometry::Builder& tpd::Geometry::Builder::vertexAttribute(
    const uint32_t location,
    const vk::Format format,
    const uint32_t stride)
{
    auto bindingDescription = VkVertexInputBindingDescription2EXT{};
    bindingDescription.sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT;
    bindingDescription.binding = static_cast<uint32_t>(_bindings.size());
    bindingDescription.stride = stride;
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    bindingDescription.divisor = 1;

    auto attributeDescription = VkVertexInputAttributeDescription2EXT{};
    attributeDescription.sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT;
    attributeDescription.binding = static_cast<uint32_t>(_bindings.size());
    attributeDescription.location = location;
    attributeDescription.format = static_cast<VkFormat>(format);
    attributeDescription.offset = 0;

    _bindings.push_back(bindingDescription);
    _attributes.push_back(attributeDescription);
    _attributeBindings.push_back(location);

    return *this;
}

tpd::Geometry::Builder& tpd::Geometry::Builder::instanceAttribute(
    const uint32_t location,
    const vk::Format format,
    const uint32_t stride,
    const uint32_t divisor)
{
    auto bindingDescription = VkVertexInputBindingDescription2EXT{};
    bindingDescription.sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT;
    bindingDescription.binding = static_cast<uint32_t>(_bindings.size());
    bindingDescription.stride = stride;
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
    bindingDescription.divisor = divisor;

    auto attributeDescription = VkVertexInputAttributeDescription2EXT{};
    attributeDescription.sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT;
    attributeDescription.binding = static_cast<uint32_t>(_bindings.size());
    attributeDescription.location = location;
    attributeDescription.format = static_cast<VkFormat>(format);
    attributeDescription.offset = 0;

    _bindings.push_back(bindingDescription);
    _attributes.push_back(attributeDescription);
    _attributeBindings.push_back(location);

    return *this;
}

std::shared_ptr<tpd::Geometry> tpd::Geometry::Builder::build(const Renderer& renderer, const vk::PrimitiveTopology topology) {
    return build(renderer.getResourceAllocator(), topology);
}

std::shared_ptr<tpd::Geometry> tpd::Geometry::Builder::build(const ResourceAllocator& allocator, const vk::PrimitiveTopology topology) {
    if (_vertexCount == 0) {
        PLOGW << "Geometry::Builder - The Geometry is being built with 0 vertices, did you forget to call Builder::vertexCount?";
    }
    if (_indexCount == 0) {
        PLOGW << "Geometry::Builder - The Geometry is being built with 0 indices, did you forget to call Builder::indexCount?";
    }

    auto vbBuilder = _bindings.empty()
        ? getVertexBufferBuilder(_vertexCount, _maxInstanceCount, DEFAULT_BINDING_DESCRIPTIONS)
        : getVertexBufferBuilder(_vertexCount, _maxInstanceCount, _bindings);
    auto vertexBuffer = vbBuilder.build(allocator, ResourceType::Dedicated);

    auto indexBuffer = Buffer::Builder()
        .bufferCount(1)
        .usage(vk::BufferUsageFlagBits::eIndexBuffer)
        .buffer(0, getIndexByteSize(_indexType) * _indexCount)
        .build(allocator, ResourceType::Dedicated);

    return std::make_shared<Geometry>(
        &allocator,
        _vertexCount,
        std::move(vertexBuffer),
        _indexCount,
        std::move(indexBuffer),
        topology,
        _indexType,
        std::move(_bindings),
        std::move(_attributes),
        std::move(_attributeBindings));
}

tpd::Buffer::Builder tpd::Geometry::Builder::getVertexBufferBuilder(
    const uint32_t vertexCount, const uint32_t instanceCount,
    const std::vector<VkVertexInputBindingDescription2EXT>& bindings)
{
    auto builder = Buffer::Builder()
        .bufferCount(static_cast<uint32_t>(bindings.size()))
        .usage(vk::BufferUsageFlagBits::eVertexBuffer);

    for (uint32_t i = 0; i < bindings.size(); i++) {
        const auto count = bindings[i].inputRate == VK_VERTEX_INPUT_RATE_VERTEX ? vertexCount : instanceCount;
        builder.buffer(i, bindings[i].stride * count);
    }

    return builder;
}

std::size_t tpd::Geometry::Builder::getIndexByteSize(const vk::IndexType type) {
    using enum vk::IndexType;
    switch (type) {
        case eUint16: return sizeof(uint16_t);
        case eUint32: return sizeof(uint32_t);
        default: throw std::runtime_error("Geometry::Builder - Unsupported index type");
    }
}

const std::vector<VkVertexInputBindingDescription2EXT> tpd::Geometry::DEFAULT_BINDING_DESCRIPTIONS{
    VkVertexInputBindingDescription2EXT{  // position
        VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT, nullptr,
        /* binding */ 0, /* stride */ sizeof(float) * 3,
        VK_VERTEX_INPUT_RATE_VERTEX, /* divisor */ 1,
    },
    VkVertexInputBindingDescription2EXT{  // normal
        VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT, nullptr,
        /* binding */ 1, /* stride */ sizeof(float) * 3,
        VK_VERTEX_INPUT_RATE_VERTEX, /* divisor */ 1,
    },
    VkVertexInputBindingDescription2EXT{  // uv
        VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT, nullptr,
        /* binding */ 2, /* stride */ sizeof(float) * 2,
        VK_VERTEX_INPUT_RATE_VERTEX, /* divisor */ 1,
    },
};

const std::vector<VkVertexInputAttributeDescription2EXT> tpd::Geometry::DEFAULT_ATTRIBUTE_DESCRIPTIONS{
    VkVertexInputAttributeDescription2EXT{  // position
        VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr,
        /* location */ 0, /* binding */ 0, VK_FORMAT_R32G32B32_SFLOAT, /* offset */ 0,
    },
    VkVertexInputAttributeDescription2EXT{  // normal
        VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr,
        /* location */ 1, /* binding */ 1, VK_FORMAT_R32G32B32_SFLOAT, /* offset */ 0,
    },
    VkVertexInputAttributeDescription2EXT{  // uv
        VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr,
        /* location */ 2, /* binding */ 2, VK_FORMAT_R32G32_SFLOAT,    /* offset */ 0,
    },
};

const std::map<std::string_view, uint32_t> tpd::Geometry::DEFAULT_ATTRIBUTE_BINDINGS{
    { "position", 0 },
    { "normal",   1 },
    { "uv",       2 },
};

void tpd::Geometry::setVertexData(const uint32_t location, const void* const data, const Renderer& renderer) const {
    const auto found = std::ranges::find(_attributeBindings, location);
    if (found == _attributeBindings.end()) {
        throw std::runtime_error(
            "Geometry - Unregistered location: if you're not using manually-defined vertex attributes, "
            "consider the overload that accepts an attribute key.");
    }
    const auto binding  = std::distance(_attributeBindings.begin(), found);
    const auto dataSize = _bindingDescriptions[binding].stride * _vertexCount;
    _vertexBuffer->transferBufferData(binding, data, dataSize, renderer.getResourceAllocator(),
        [&renderer](const auto src, const auto dst, const auto& bufferCopy) { renderer.copyBufferToBuffer(src, dst, bufferCopy); });
}

void tpd::Geometry::setVertexData(
    const uint32_t location,
    const void* const data,
    const std::size_t dataSize,
    const Renderer& renderer) const
{
    const auto found = std::ranges::find(_attributeBindings, location);
    if (found == _attributeBindings.end()) {
        throw std::runtime_error(
            "Geometry - Unregistered location: if you're not using manually-defined vertex attributes, "
            "consider the overload that accepts an attribute key.");
    }
    const auto binding = std::distance(_attributeBindings.begin(), found);
    _vertexBuffer->transferBufferData(binding, data, dataSize, renderer.getResourceAllocator(),
        [&renderer](const auto src, const auto dst, const auto& bufferCopy) { renderer.copyBufferToBuffer(src, dst, bufferCopy); });
}

void tpd::Geometry::setVertexData(
    const std::string_view attribute,
    const void* const data,
    const Renderer& renderer) const
{
    if (!DEFAULT_ATTRIBUTE_BINDINGS.contains(attribute)) {
        throw std::runtime_error(
            "Geometry - Unrecognized attribute key: if you're using manually-defined vertex attributes, "
            "consider the overload that accepts a location parameter.");
    }
    const auto binding  = DEFAULT_ATTRIBUTE_BINDINGS.at(attribute);
    const auto dataSize = DEFAULT_BINDING_DESCRIPTIONS[binding].stride * _vertexCount;
    _vertexBuffer->transferBufferData(binding, data, dataSize, renderer.getResourceAllocator(),
        [&renderer](const auto src, const auto dst, const auto& bufferCopy) { renderer.copyBufferToBuffer(src, dst, bufferCopy); });
}

void tpd::Geometry::setIndexData(const void* const data, const std::size_t dataSize, const Renderer& renderer) const {
    _indexBuffer->transferBufferData(
        0, data, dataSize, renderer.getResourceAllocator(),
        [&renderer](const auto src, const auto dst, const auto& bufferCopy) { renderer.copyBufferToBuffer(src, dst, bufferCopy); });
}

void tpd::Geometry::setIndexData(const void* const data, const Renderer& renderer) const {
    setIndexData(data, sizeof(uint32_t) * _indexCount, renderer);
}

void tpd::Geometry::dispose() noexcept {
    if (_vertexBuffer) {
        _vertexBuffer->dispose(*_allocator);
        _vertexBuffer.reset();
    }
    if (_indexBuffer) {
        _indexBuffer->dispose(*_allocator);
        _indexBuffer.reset();
    }
}

tpd::Geometry::~Geometry() {
    if (_allocator) dispose();
    _bindingDescriptions.clear();
    _attributeDescriptions.clear();
    _attributeBindings.clear();
}
