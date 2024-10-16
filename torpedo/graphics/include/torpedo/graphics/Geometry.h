#pragma once

#include <torpedo/foundation/Buffer.h>

#include <map>

namespace tpd {
    class Engine;

    enum class VertexAttribute : uint32_t {
        Position = 0,
        Normal   = 1,
        UV       = 2,
        Color    = 3,
    };

    class Geometry {
    public:
        class Builder {
        public:
            /**
             * Constructs a new Builder with specified vertex, index, and instance counts.
             *
             * @param vertexCount The number of vertices in the geometry.
             * @param indexCount The number of indices in the geometry.
             * @param instanceCount The number of instances (for instanced rendering).
             */
            Builder(uint32_t vertexCount, uint32_t indexCount, uint32_t instanceCount = 1);

            /**
             * Registers the number of manually-defined vertex attributes. This method should be called before chaining
             * vertexAttribute/instanceAttribute calls to better optimize runtime performance.
             *
             * @param count The total number of separated attributes this Geometry will have.
             * @return This Builder object for chaining calls.
             */
            Builder& attributeCount(uint32_t count);

            /**
             * Manually defines a vertex attribute for a given location with the specified format and byte stride.
             * Note that a call to this method disables the use of default vertex attributes.
             *
             * @param location The location declared for the vertex attribute in the shader.
             * @param format The format of the vertex attribute (e.g., vk::Format).
             * @param stride The byte stride between consecutive vertex elements of this attribute.
             * @return This Builder object for chaining calls.
             */
            Builder& vertexAttribute(uint32_t location, vk::Format format, uint32_t stride);

            /**
             * Manually defines a vertex attribute using its default location, format, and stride. This is useful when
             * the Geometry requires a non-default vertex attribute (like color) and it is being used with Materials
             * that define their own internal shaders. Note that a call to this method disables the use of default
             * vertex attributes.
             *
             * @param attribute The predefined vertex attribute.
             * @return This Builder object for chaining calls.
             */
            Builder& vertexAttribute(VertexAttribute attribute);

            /**
             * Manually defines a vertex attribute that varies per instance. Note that a call to this method disables
             * the use of default vertex attributes.
             *
             * @param location The location declared for the vertex attribute in the shader.
             * @param format The format of the vertex attribute (e.g., vk::Format).
             * @param stride The byte stride between consecutive instance elements of this attribute.
             * @param divisor The divisor for instanced rendering.
             * @return This Builder object for chaining calls.
             */
            Builder& instanceAttribute(uint32_t location, vk::Format format, uint32_t stride, uint32_t divisor = 1);

            /**
             * Builds the Geometry.
             *
             * @param engine The Engine that will manage this Geometry's resource allocation.
             * @return A shared pointer to the constructed Geometry object.
             */
            [[nodiscard]] std::shared_ptr<Geometry> build(const Engine& engine);

        private:
            static Buffer::Builder getVertexBufferBuilder(
                uint32_t vertexCount, uint32_t instanceCount,
                const std::vector<VkVertexInputBindingDescription2EXT>& bindings);

            static vk::Format getDefaultVertexAttributeFormat(VertexAttribute attribute);
            static uint32_t getDefaultVertexAttributeStride(VertexAttribute attribute);

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

        /**
         * Transfers vertex data to the vertex attribute specified at location. This method shall be used if the
         * Geometry was created with manually-defined vertex attributes.
         *
         * @param location The location declared for the vertex attribute in the shader.
         * @param data Pointer to the data array.
         * @param dataSize Total byte size of the data pointer. A value of stride * vertexCount will exhaust this attribute's buffer slot.
         * @param engine The Engine that was used to create this Geometry.
         */
        void setVertexData(uint32_t location, const void* data, std::size_t dataSize, const Engine& engine) const;

        /**
         * Transfers vertex data to the vertex attribute specified at by attribute key. This method shall be used if the
         * Geometry was created without any manually-defined vertex attributes.
         *
         * @param attribute A string key to one of the default attributes.
         * @param data Pointer to the data array.
         * @param engine The Engine that was used to create this Geometry.
         */
        void setVertexData(std::string_view attribute, const void* data, const Engine& engine) const;

        /**
         * Transfers index data to the local device buffer associated with this Geometry.
         *
         * @param data Pointer to the data array. Note that Geometry uses uint32_t index data type.
         * @param dataSize Total byte size of the data pointer. Note that index values are uint32_t by default.
         * @param engine The Engine that was used to create this Geometry.
         */
        void setIndexData(const void* data, std::size_t dataSize, const Engine& engine) const;

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
