#pragma once

#include <torpedo/foundation/Buffer.h>
#include <torpedo/foundation/ResourceAllocator.h>

#include <map>

namespace tpd {
    class Geometry final {
        static const ResourceAllocator* _allocator;
        friend class Renderer;

    public:
        class Builder {
        public:
            /**
             * Specifies the number of vertices in the vertex buffer.
             *
             * @return This Builder object for chaining calls.
             */
            Builder& vertexCount(uint32_t count);

            /**
             * Specifies the number of indices in the index buffer.
             *
             * @return This Builder object for chaining calls.
             */
            Builder& indexCount(uint32_t count);

            /**
             * Specifies the maximum number of instances (for instanced rendering).
             *
             * @return This Builder object for chaining calls.
             */
            Builder& maxInstanceCount(uint32_t count);

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
             * Specifies the index type, default to uint32_t.
             *
             * @return This Builder object for chaining calls.
             */
            Builder& indexType(vk::IndexType type);

            /**
             * Builds the Geometry.
             *
             * @param renderer The Renderer that will manage this Geometry's resources.
             * @param topology The topology that will be used to draw this Geometry, default to TriangleList.
             * @return A shared pointer to the constructed Geometry object.
             */
            [[nodiscard]] std::shared_ptr<Geometry> build(const Renderer& renderer, vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList);

            /**
             * Builds the Geometry.
             *
             * @param allocator A ResourceAllocator, which can be retrieved from a Renderer or an independent one.
             * @param topology The topology that will be used to draw this Geometry, default to TriangleList.
             * @return A shared pointer to the constructed Geometry object.
             */
            [[nodiscard]] std::shared_ptr<Geometry> build(const ResourceAllocator& allocator, vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList);

        private:
            static Buffer::Builder getVertexBufferBuilder(
                uint32_t vertexCount, uint32_t instanceCount,
                const std::vector<VkVertexInputBindingDescription2EXT>& bindings);

            static std::size_t getIndexByteSize(vk::IndexType type);

            uint32_t _vertexCount{ 0 };
            uint32_t _indexCount{ 0 };
            uint32_t _maxInstanceCount{ 1 };
            vk::IndexType _indexType{ vk::IndexType::eUint32 };

            std::vector<VkVertexInputBindingDescription2EXT> _bindings{};
            std::vector<VkVertexInputAttributeDescription2EXT> _attributes{};
            std::vector<uint32_t> _attributeBindings{};
        };

        Geometry(
            uint32_t vertexCount, std::unique_ptr<Buffer> vertexBuffer,
            uint32_t indexCount, std::unique_ptr<Buffer> indexBuffer,
            vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList,
            vk::IndexType indexType = vk::IndexType::eUint32,
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
         * @param data Pointer to the data array whose size exhausts this attribute's buffer slot.
         * @param renderer The Renderer that was used/owning the ResourceAllocator when creating this Geometry.
         */
        void setVertexData(uint32_t location, const void* data, const Renderer& renderer) const;

        /**
         * Transfers vertex data to the vertex attribute specified at location. This method shall be used if the
         * Geometry was created with manually-defined vertex attributes.
         *
         * This overload accepts an additional dataSize parameter to designate the size of the data pointer.
         *
         * @param location The location declared for the vertex attribute in the shader.
         * @param data Pointer to the data array.
         * @param dataSize Total byte size of the data pointer. A value of stride * vertexCount will exhaust this attribute's buffer slot.
         * @param renderer The Renderer that was used/owning the ResourceAllocator when creating this Geometry.
         */
        void setVertexData(uint32_t location, const void* data, std::size_t dataSize, const Renderer& renderer) const;

        /**
         * Transfers vertex data to the vertex attribute specified by an attribute key. This method shall be used if the
         * Geometry was created without any manually-defined vertex attributes. The available keys are:
         * - position
         * - normal
         * - uv
         *
         * @param attribute A string key to one of the default attributes.
         * @param data Pointer to the data array.
         * @param renderer The Renderer that was used/owning the ResourceAllocator when creating this Geometry.
         */
        void setVertexData(std::string_view attribute, const void* data, const Renderer& renderer) const;

        /**
         * Transfers index data to the local device buffer associated with this Geometry.
         *
         * @param data Pointer to the data array.
         * @param dataSize Total byte size of the data pointer. Note that index values are uint32_t by default, unless
         * specified otherwise with Geometry::Builder::indexType().
         * @param renderer The Renderer that was used/owning the ResourceAllocator when creating this Geometry.
         */
        void setIndexData(const void* data, std::size_t dataSize, const Renderer& renderer) const;

        /**
         * Transfers index data to the local device buffer associated with this Geometry. This overload assumes the
         * Geometry was built with 4-byte index element type (uint32_t).
         *
         * @param data Pointer to the data array.
         * @param renderer The Renderer that was used/owning the ResourceAllocator when creating this Geometry.
         */
        void setIndexData(const void* data, const Renderer& renderer) const;

        [[nodiscard]] uint32_t getVertexCount() const noexcept;
        [[nodiscard]] const std::unique_ptr<Buffer>& getVertexBuffer() const noexcept;

        [[nodiscard]] uint32_t getIndexCount()  const noexcept;
        [[nodiscard]] const std::unique_ptr<Buffer>& getIndexBuffer() const noexcept;

        [[nodiscard]] vk::PrimitiveTopology getPrimitiveTopology() const noexcept;
        [[nodiscard]] vk::IndexType getIndexType() const noexcept;

        [[nodiscard]] const std::vector<VkVertexInputBindingDescription2EXT>& getBindingDescriptions() const noexcept;
        [[nodiscard]] const std::vector<VkVertexInputAttributeDescription2EXT>& getAttributeDescriptions() const noexcept;

        /**
         * Frees GPU-side memory of this Geometry, using the ResourceAllocator associated with
         * the Renderer that was used to allocate this object's resources.
         */
        void dispose() noexcept;

        /**
         * Frees GPU-side memory associated with this Geometry.
         *
         * @param allocator the ResourceAllocator that was used to allocate this Geometry's resources.
         */
        void dispose(const ResourceAllocator& allocator) noexcept;

        ~Geometry();

    private:
        uint32_t _vertexCount;
        std::unique_ptr<Buffer> _vertexBuffer;

        uint32_t _indexCount;
        std::unique_ptr<Buffer> _indexBuffer;

        vk::PrimitiveTopology _primitiveTopology;
        vk::IndexType _indexType;

        std::vector<VkVertexInputBindingDescription2EXT> _bindingDescriptions;
        std::vector<VkVertexInputAttributeDescription2EXT> _attributeDescriptions;
        std::vector<uint32_t> _attributeBindings;

        static const std::vector<VkVertexInputBindingDescription2EXT> DEFAULT_BINDING_DESCRIPTIONS;
        static const std::vector<VkVertexInputAttributeDescription2EXT> DEFAULT_ATTRIBUTE_DESCRIPTIONS;
        static const std::map<std::string_view, uint32_t> DEFAULT_ATTRIBUTE_BINDINGS;
    };
}

inline const tpd::ResourceAllocator* tpd::Geometry::_allocator = nullptr;

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::Geometry::Builder& tpd::Geometry::Builder::vertexCount(const uint32_t count) {
    _vertexCount = count;
    return *this;
}

inline tpd::Geometry::Builder& tpd::Geometry::Builder::indexCount(const uint32_t count) {
    _indexCount = count;
    return *this;
}

inline tpd::Geometry::Builder& tpd::Geometry::Builder::maxInstanceCount(const uint32_t count) {
    _maxInstanceCount = count;
    return *this;
}

inline tpd::Geometry::Builder& tpd::Geometry::Builder::indexType(const vk::IndexType type) {
    _indexType = type;
    return *this;
}

inline tpd::Geometry::Geometry(
    const uint32_t vertexCount,
    std::unique_ptr<Buffer> vertexBuffer,
    const uint32_t indexCount,
    std::unique_ptr<Buffer> indexBuffer,
    const vk::PrimitiveTopology topology,
    const vk::IndexType indexType,
    std::vector<VkVertexInputBindingDescription2EXT>&& bindingDescriptions,
    std::vector<VkVertexInputAttributeDescription2EXT>&& attributeDescriptions,
    std::vector<uint32_t>&& attributeBindings) noexcept
    : _vertexCount{ vertexCount }
    , _vertexBuffer{ std::move(vertexBuffer) }
    , _indexCount{ indexCount }
    , _indexBuffer{ std::move(indexBuffer) }
    , _primitiveTopology{ topology }
    , _indexType{ indexType }
    , _bindingDescriptions{ std::move(bindingDescriptions) }
    , _attributeDescriptions{ std::move(attributeDescriptions) }
    , _attributeBindings{ std::move(attributeBindings) } {
}

inline uint32_t tpd::Geometry::getVertexCount() const noexcept {
    return _vertexCount;
}

inline const std::unique_ptr<tpd::Buffer>& tpd::Geometry::getVertexBuffer() const noexcept {
    return _vertexBuffer;
}

inline uint32_t tpd::Geometry::getIndexCount() const noexcept {
    return _indexCount;
}

inline const std::unique_ptr<tpd::Buffer>& tpd::Geometry::getIndexBuffer() const noexcept {
    return _indexBuffer;
}

inline vk::PrimitiveTopology tpd::Geometry::getPrimitiveTopology() const noexcept {
    return _primitiveTopology;
}

inline vk::IndexType tpd::Geometry::getIndexType() const noexcept {
    return _indexType;
}

inline const std::vector<VkVertexInputBindingDescription2EXT>& tpd::Geometry::getBindingDescriptions() const noexcept {
    return _bindingDescriptions.empty() ? DEFAULT_BINDING_DESCRIPTIONS : _bindingDescriptions;
}

inline const std::vector<VkVertexInputAttributeDescription2EXT>& tpd::Geometry::getAttributeDescriptions() const noexcept {
    return _attributeDescriptions.empty() ? DEFAULT_ATTRIBUTE_DESCRIPTIONS : _attributeDescriptions;
}
