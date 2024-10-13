#pragma once

#include <torpedo/foundation/Buffer.h>

#include <map>

namespace tpd {
    class Engine;

    class Geometry {
    public:
        class Builder {
        public:
            Builder(uint32_t vertexCount, uint32_t indexCount, uint32_t instanceCount = 1);

            template<typename T = Builder>
            T& attributeCount(uint32_t count);

            template<typename T = Builder>
            T& vertexAttribute(uint32_t location, vk::Format format, uint32_t byteStride);

            template<typename T = Builder>
            T& instanceAttribute(uint32_t location, vk::Format format, uint32_t byteStride, uint32_t divisor = 1);

            [[nodiscard]] std::shared_ptr<Geometry> build(const Engine& engine);

        private:
            static Buffer::Builder getVertexBufferBuilder(
                uint32_t vertexCount, uint32_t instanceCount,
                const std::vector<VkVertexInputBindingDescription2EXT>& bindings);

            uint32_t _vertexCount;
            uint32_t _indexCount;
            uint32_t _instanceCount;

            std::vector<VkVertexInputBindingDescription2EXT> _bindings{};
            std::vector<VkVertexInputAttributeDescription2EXT> _attributes{};
            std::vector<uint32_t> _attributeBindings{};
        };

        Geometry(
            uint32_t vertexCount, std::unique_ptr<Buffer> vertexBuffer,
            uint32_t indexCount, std::unique_ptr<Buffer> indexBuffer,
            std::vector<VkVertexInputBindingDescription2EXT>&& bindingDescriptions = {},
            std::vector<VkVertexInputAttributeDescription2EXT>&& attributeDescriptions = {},
            std::vector<uint32_t>&& attributeBindings = {}) noexcept;

        Geometry(const Geometry&) = delete;
        Geometry& operator=(const Geometry&) = delete;

        void setVertexData(uint32_t location,          const void* data, std::size_t dataByteSize, const Engine& engine) const;
        void setVertexData(std::string_view attribute, const void* data, std::size_t dataByteSize, const Engine& engine) const;

        void setIndexData(const void* data, std::size_t dataByteSize, const Engine& engine) const;

        [[nodiscard]] uint32_t getVertexCount() const noexcept;
        [[nodiscard]] uint32_t getIndexCount()  const noexcept;

        [[nodiscard]] const std::vector<VkVertexInputBindingDescription2EXT>& getBindingDescriptions() const noexcept;
        [[nodiscard]] const std::vector<VkVertexInputAttributeDescription2EXT>& getAttributeDescriptions() const noexcept;

        void dispose(const Engine& engine) noexcept;

    private:
        uint32_t _vertexCount;
        std::unique_ptr<Buffer> _vertexBuffer;

        uint32_t _indexCount;
        std::unique_ptr<Buffer> _indexBuffer;

        std::vector<VkVertexInputBindingDescription2EXT> _bindingDescriptions;
        std::vector<VkVertexInputAttributeDescription2EXT> _attributeDescriptions;
        std::vector<uint32_t> _attributeBindings;

        static const std::vector<VkVertexInputBindingDescription2EXT> DEFAULT_BINDING_DESCRIPTIONS;
        static const std::vector<VkVertexInputAttributeDescription2EXT> DEFAULT_ATTRIBUTE_DESCRIPTIONS;
        static const std::map<std::string_view, uint32_t> DEFAULT_ATTRIBUTE_BINDINGS;
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::Geometry::Builder::Builder(const uint32_t vertexCount, const uint32_t indexCount, const uint32_t instanceCount)
    : _vertexCount{ vertexCount }, _indexCount { indexCount }, _instanceCount{ instanceCount } {
}

template<typename T>
T& tpd::Geometry::Builder::attributeCount(const uint32_t count) {
    _bindings.reserve(count);
    _attributes.reserve(count);
    _attributeBindings.reserve(count);
    return *static_cast<T*>(this);
}

template<typename T>
T& tpd::Geometry::Builder::vertexAttribute(
    const uint32_t location,
    const vk::Format format,
    const uint32_t byteStride)
{
    auto bindingDescription = VkVertexInputBindingDescription2EXT{};
    bindingDescription.sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT;
    bindingDescription.binding = static_cast<uint32_t>(_bindings.size());
    bindingDescription.stride = byteStride;
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

    return *static_cast<T*>(this);
}

template<typename T>
T& tpd::Geometry::Builder::instanceAttribute(
    const uint32_t location,
    const vk::Format format,
    const uint32_t byteStride,
    const uint32_t divisor)
{
    auto bindingDescription = VkVertexInputBindingDescription2EXT{};
    bindingDescription.sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT;
    bindingDescription.binding = static_cast<uint32_t>(_bindings.size());
    bindingDescription.stride = byteStride;
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

    return *static_cast<T*>(this);
}

inline tpd::Geometry::Geometry(
    const uint32_t vertexCount,
    std::unique_ptr<Buffer> vertexBuffer,
    const uint32_t indexCount,
    std::unique_ptr<Buffer> indexBuffer,
    std::vector<VkVertexInputBindingDescription2EXT>&& bindingDescriptions,
    std::vector<VkVertexInputAttributeDescription2EXT>&& attributeDescriptions,
    std::vector<uint32_t>&& attributeBindings) noexcept
    : _vertexCount{ vertexCount }
    , _vertexBuffer{ std::move(vertexBuffer) }
    , _indexCount{ indexCount }
    , _indexBuffer{ std::move(indexBuffer) }
    , _bindingDescriptions{ std::move(bindingDescriptions) }
    , _attributeDescriptions{ std::move(attributeDescriptions) }
    , _attributeBindings{ std::move(attributeBindings) } {
}

inline uint32_t tpd::Geometry::getVertexCount() const noexcept {
    return _vertexCount;
}

inline uint32_t tpd::Geometry::getIndexCount() const noexcept {
    return _indexCount;
}

inline const std::vector<VkVertexInputBindingDescription2EXT>& tpd::Geometry::getBindingDescriptions() const noexcept {
    return _bindingDescriptions.empty() ? DEFAULT_BINDING_DESCRIPTIONS : _bindingDescriptions;
}

inline const std::vector<VkVertexInputAttributeDescription2EXT>& tpd::Geometry::getAttributeDescriptions() const noexcept {
    return _attributeDescriptions.empty() ? DEFAULT_ATTRIBUTE_DESCRIPTIONS : _attributeDescriptions;
}
