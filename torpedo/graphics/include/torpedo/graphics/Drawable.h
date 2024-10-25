#pragma once

#include "torpedo/graphics/Composable.h"
#include "torpedo/graphics/Geometry.h"
#include "torpedo/graphics/MaterialInstance.h"

namespace tpd {
    class Drawable final : public Composable {
    public:
        struct Mesh {
            std::shared_ptr<Geometry> geometry{};
            std::shared_ptr<MaterialInstance> materialInstance{};
            uint32_t instanceCount{ 0 };
            uint32_t firstInstance{ 0 };
            int32_t vertexOffset{ 0 };
            const Drawable* drawable{ nullptr };
        };

        class Builder {
        public:
            /**
             * Tells the Builder how many meshes the Drawable is going to have.
             * @param count The number of meshes the Drawable is going to hold.
             * @return This Builder object for chaining calls.
             */
            Builder& meshCount(uint32_t count);

            /**
             * Adds a Mesh to include in the Drawable. All meshes within the same Drawable share transform properties.
             *
             * @param index The index of the Mesh, which can be later used to change drawing settings.
             * @param geometry A shared pointer to the Geometry containing vertex and index buffer to be consumed.
             * @param materialInstance A shared pointer to the material instance associated with the Geometry.
             * @param instanceCount The number of instances to draw for this Mesh.
             * @param firstInstance The first instance to draw for this Mesh.
             * @param vertexOffset The value added to the vertex index before indexing into the vertex buffer.
             * @return This Builder object for chaining calls.
             */
            Builder& mesh(
                uint32_t index,
                const std::shared_ptr<Geometry>& geometry,
                const std::shared_ptr<MaterialInstance>& materialInstance,
                uint32_t instanceCount = 1, uint32_t firstInstance = 0, int32_t vertexOffset = 0);

            /**
             * Builds and returns a drawable object containing all specified Meshes.
             *
             * @return A shared pointer to the Drawable object sharing the same transform.
             */
            [[nodiscard]] std::shared_ptr<Drawable> build();

        private:
            std::vector<Mesh> _meshes{};
        };

        explicit Drawable(std::vector<Mesh>&& meshes) noexcept;

        Drawable(const Drawable&) = delete;
        Drawable& operator=(const Drawable&) = delete;

        void setInstanceDraw(uint32_t meshIndex, uint32_t instanceCount, uint32_t firstInstance);
        void setVertexOffset(uint32_t meshIndex, int32_t offset);

        const std::vector<Mesh>& getMeshes() const;

        struct DrawableObject {
            alignas(16) glm::mat4 transform;
            alignas(16) glm::mat4 normalMat;
        };

    private:
        std::vector<Mesh> _meshes{};
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::Drawable::Builder& tpd::Drawable::Builder::meshCount(const uint32_t count) {
    _meshes.resize(count);
    return *this;
}

inline tpd::Drawable::Builder& tpd::Drawable::Builder::mesh(
    const uint32_t index,
    const std::shared_ptr<Geometry>& geometry,
    const std::shared_ptr<MaterialInstance>& materialInstance,
    const uint32_t instanceCount,
    const uint32_t firstInstance,
    const int32_t vertexOffset)
{
    _meshes[index] = { geometry, materialInstance, instanceCount, firstInstance, vertexOffset, nullptr };
    return *this;
}

inline tpd::Drawable::Drawable(std::vector<Mesh>&& meshes) noexcept : _meshes{ std::move(meshes) } {
    for (auto& mesh : _meshes) {
        mesh.drawable = this;
    }
}

inline void tpd::Drawable::setInstanceDraw(const uint32_t meshIndex, const uint32_t instanceCount, const uint32_t firstInstance) {
    _meshes[meshIndex].instanceCount = instanceCount;
    _meshes[meshIndex].firstInstance = firstInstance;
}

inline void tpd::Drawable::setVertexOffset(const uint32_t meshIndex, const int32_t offset) {
    _meshes[meshIndex].vertexOffset = offset;
}

inline const std::vector<tpd::Drawable::Mesh>& tpd::Drawable::getMeshes() const {
    return _meshes;
}
