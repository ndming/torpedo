#include "torpedo/graphics/Geometry.h"

#include "torpedo/graphics/Engine.h"

std::shared_ptr<tpd::Geometry> tpd::Geometry::Builder::build(const Engine& engine) {
    auto vbBuilder = _bindings.empty()
        ? getVertexBufferBuilder(_vertexCount, _instanceCount, DEFAULT_BINDING_DESCRIPTIONS)
        : getVertexBufferBuilder(_vertexCount, _instanceCount, _bindings);
    auto vertexBuffer = vbBuilder.build(engine.getResourceAllocator(), ResourceType::Dedicated);

    auto indexBuffer = Buffer::Builder()
        .bufferCount(1)
        .usage(vk::BufferUsageFlagBits::eIndexBuffer)
        .buffer(0, sizeof(uint32_t) * _indexCount)
        .build(engine.getResourceAllocator(), ResourceType::Dedicated);

    return std::make_shared<Geometry>(
        _vertexCount, std::move(vertexBuffer),
        _indexCount, std::move(indexBuffer),
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

const std::vector<VkVertexInputBindingDescription2EXT> tpd::Geometry::DEFAULT_BINDING_DESCRIPTIONS{
    VkVertexInputBindingDescription2EXT{  // positions
        VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT, nullptr,
        /* binding */ 0, /* stride */ sizeof(float) * 3,
        VK_VERTEX_INPUT_RATE_VERTEX, /* divisor */ 1,
    },
    VkVertexInputBindingDescription2EXT{  // normals
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
    VkVertexInputAttributeDescription2EXT{  // positions
        VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr,
        /* location */ 0, /* binding */ 0, VK_FORMAT_R32G32B32_SFLOAT, /* offset */ 0,
    },
    VkVertexInputAttributeDescription2EXT{  // normals
        VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr,
        /* location */ 1, /* binding */ 1, VK_FORMAT_R32G32B32_SFLOAT, /* offset */ 0,
    },
    VkVertexInputAttributeDescription2EXT{  // uv
        VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr,
        /* location */ 2, /* binding */ 2, VK_FORMAT_R32G32_SFLOAT,    /* offset */ 0,
    },
};

const std::map<std::string_view, uint32_t> tpd::Geometry::DEFAULT_ATTRIBUTE_BINDINGS{
    { "positions", 0 },
    { "normals",   1 },
    { "uv",        2 },
};

void tpd::Geometry::setVertexData(
    const uint32_t location,
    const void* const data,
    const std::size_t dataByteSize,
    const Engine& engine) const
{
    const auto found = std::ranges::find(_attributeBindings, location);
    if (found == _attributeBindings.end()) {
        throw std::runtime_error(
            "Geometry - Unregistered location: if you're not specifying custom vertex attributes, "
            "consider the overload that accepts an attribute key.");
    }
    const auto binding = std::distance(_attributeBindings.begin(), found);
    _vertexBuffer->transferBufferData(
        binding, data, dataByteSize, engine.getResourceAllocator(),
        [&engine](const auto src, const auto dst, const auto& bufferCopy) { engine.copyBuffer(src, dst, bufferCopy); });
}

void tpd::Geometry::setVertexData(
    const std::string_view attribute,
    const void* const data,
    const std::size_t dataByteSize,
    const Engine& engine) const
{
    if (!DEFAULT_ATTRIBUTE_BINDINGS.contains(attribute)) {
        throw std::runtime_error(
            "Geometry - Unrecognized attribute key: if you're using a custom vertex attribute, "
            "consider the overload that accepts a location parameter.");
    }
    const auto binding = DEFAULT_ATTRIBUTE_BINDINGS.at(attribute);
    _vertexBuffer->transferBufferData(
        binding, data, dataByteSize, engine.getResourceAllocator(),
        [&engine](const auto src, const auto dst, const auto& bufferCopy) { engine.copyBuffer(src, dst, bufferCopy); });
}

void tpd::Geometry::setIndexData(const void* const data, const std::size_t dataByteSize, const Engine& engine) const {
    _indexBuffer->transferBufferData(
        0, data, dataByteSize, engine.getResourceAllocator(),
        [&engine](const auto src, const auto dst, const auto& bufferCopy) { engine.copyBuffer(src, dst, bufferCopy); });
}

void tpd::Geometry::dispose(const Engine& engine) noexcept {
    _vertexBuffer->dispose(engine.getResourceAllocator());
    _indexBuffer->dispose(engine.getResourceAllocator());
    _vertexBuffer.reset();
    _indexBuffer.reset();
    _bindingDescriptions.clear();
    _attributeDescriptions.clear();
    _attributeBindings.clear();
}
