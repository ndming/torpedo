#pragma once

#include "torpedo/graphics/Composable.h"
#include "torpedo/graphics/Geometry.h"
#include "torpedo/graphics/MaterialInstance.h"

namespace tpd {
    class Drawable final : public Composable {
    public:
        class Builder {
        public:
            /**
             * Sets the number of indices to be used for indexed drawing.
             *
             * @param count The number of indices to be used. An indexCount of zero will take all indices
             * in the index buffer to draw the Drawable.
             * @return This Builder object for chaining calls.
             */
            Builder& indexCount(uint32_t count);

            /**
             * Sets the number of instances to draw.
             *
             * @param count The number of instances to draw in a single call.
             * @return This Builder object for chaining calls.
             */
            Builder& instanceCount(uint32_t count);

            /**
             * Sets the first index in the index buffer to start drawing from.
             *
             * @param index The starting index in the index buffer for the draw call.
             * @return This Builder object for chaining calls.
             */
            Builder& firstIndex(uint32_t index);

            /**
             * Specifies the value added to the vertex index before indexing into the vertex buffer.
             *
             * @param offset Offset added to indices before being supplied as the vertexIndex value.
             * @return This Builder object for chaining calls.
             */
            Builder& vertexOffset(int32_t offset);

            /**
             * Sets the first instance to draw.
             *
             * @param instance The instance ID of the first instance to draw.
             * @return This Builder object for chaining calls.
             */
            Builder& firstInstance(uint32_t instance);

            /**
             * Builds and returns a drawable object with the specified geometry and material.
             *
             * @param geometry A shared pointer to the Geometry containing vertex and index buffer to be consumed.
             * @param materialInstance A shared pointer to the material instance to be used.
             * @return A shared pointer to a Drawable object configured with the set parameters.
             */
            [[nodiscard]] std::shared_ptr<Drawable> build(
                const std::shared_ptr<Geometry>& geometry,
                const std::shared_ptr<MaterialInstance>& materialInstance) const;

        private:
            uint32_t _indexCount{ 0 };
            uint32_t _instanceCount{ 1 };
            uint32_t _firstIndex{ 0 };
            int32_t  _vertexOffset{ 0 };
            uint32_t _firstInstance{ 0 };

        };

        Drawable(
            std::shared_ptr<Geometry> geometry, std::shared_ptr<MaterialInstance> materialInstance,
            uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);

        Drawable(const Drawable&) = delete;
        Drawable& operator=(const Drawable&) = delete;

        /**
         * Sets the number of indices to be used for indexed drawing. An indexCount of zero will take all indices
         * in the index buffer to draw the Drawable. Use instanceCount instead to temporarily disable the drawing
         * of this Drawable.
         *
         * @param count The number of vertices to draw.
         */
        void setIndexCount(uint32_t count);

        /**
         * The number of instances to draw.
         */
        uint32_t instanceCount;

        /**
         * The base index within the index buffer.
         */
        uint32_t firstIndex;

        /**
         * The value added to the vertex index before indexing into the vertex buffer.
         */
        int32_t vertexOffset;

        /**
         * The instance ID of the first instance to draw.
         */
        uint32_t firstInstance;

        [[nodiscard]] const std::shared_ptr<Geometry>& getGeometry() const;
        [[nodiscard]] const std::shared_ptr<MaterialInstance>& getMaterialInstance() const;
        [[nodiscard]] uint32_t getIndexCount() const;

        struct DrawableObject {
            alignas(16) glm::mat4 transform;
            alignas(16) glm::mat4 normalMat;
        };

    private:
        std::shared_ptr<Geometry> _geometry;
        std::shared_ptr<MaterialInstance> _materialInstance;

        // The actual number of indices to draw. This may not be the same as the number of indices
        // in the Geometry's index buffer.
        uint32_t _indexCount;
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::Drawable::Builder& tpd::Drawable::Builder::indexCount(const uint32_t count) {
    _indexCount = count;
    return *this;
}

inline tpd::Drawable::Builder& tpd::Drawable::Builder::instanceCount(const uint32_t count) {
    _instanceCount = count;
    return *this;
}

inline tpd::Drawable::Builder& tpd::Drawable::Builder::firstIndex(const uint32_t index) {
    _firstIndex = index;
    return *this;
}

inline tpd::Drawable::Builder& tpd::Drawable::Builder::vertexOffset(const int32_t offset) {
    _vertexOffset = offset;
    return *this;
}

inline tpd::Drawable::Builder& tpd::Drawable::Builder::firstInstance(const uint32_t instance) {
    _firstInstance = instance;
    return *this;
}

inline tpd::Drawable::Drawable(
    std::shared_ptr<Geometry> geometry,
    std::shared_ptr<MaterialInstance> materialInstance,
    const uint32_t indexCount,
    const uint32_t instanceCount,
    const uint32_t firstIndex,
    const int32_t vertexOffset,
    const uint32_t firstInstance)
    : instanceCount{ instanceCount }
    , firstIndex{ firstIndex }
    , vertexOffset{ vertexOffset }
    , firstInstance{ firstInstance }
    , _geometry{ std::move(geometry) }
    , _materialInstance{ std::move(materialInstance) }
    , _indexCount{ indexCount } {
}

inline void tpd::Drawable::setIndexCount(const uint32_t count) {
    _indexCount = count == 0 ? _geometry->getIndexCount() : count;
}

inline const std::shared_ptr<tpd::Geometry>& tpd::Drawable::getGeometry() const {
    return _geometry;
}

inline const std::shared_ptr<tpd::MaterialInstance>& tpd::Drawable::getMaterialInstance() const {
    return _materialInstance;
}

inline uint32_t tpd::Drawable::getIndexCount() const {
    return _indexCount;
}
