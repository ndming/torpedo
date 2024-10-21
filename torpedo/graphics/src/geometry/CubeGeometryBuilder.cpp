#include "torpedo/geometry/CubeGeometryBuilder.h"
#include "torpedo/graphics/Renderer.h"

std::shared_ptr<tpd::Geometry> tpd::CubeGeometryBuilder::build(const Renderer& renderer) const {
    const auto positions = std::array{
        // Face +X
         _halfExtent, -_halfExtent,  _halfExtent,
         _halfExtent, -_halfExtent, -_halfExtent,
         _halfExtent,  _halfExtent,  _halfExtent,
         _halfExtent,  _halfExtent, -_halfExtent,
        // Face +Y
         _halfExtent,  _halfExtent,  _halfExtent,
         _halfExtent,  _halfExtent, -_halfExtent,
        -_halfExtent,  _halfExtent,  _halfExtent,
        -_halfExtent,  _halfExtent, -_halfExtent,
        // Face +Z
        -_halfExtent,  _halfExtent,  _halfExtent,
        -_halfExtent, -_halfExtent,  _halfExtent,
         _halfExtent,  _halfExtent,  _halfExtent,
         _halfExtent, -_halfExtent,  _halfExtent,
        // Face -X
        -_halfExtent,  _halfExtent,  _halfExtent,
        -_halfExtent,  _halfExtent, -_halfExtent,
        -_halfExtent, -_halfExtent,  _halfExtent,
        -_halfExtent, -_halfExtent, -_halfExtent,
        // Face -Y
        -_halfExtent, -_halfExtent,  _halfExtent,
        -_halfExtent, -_halfExtent, -_halfExtent,
         _halfExtent, -_halfExtent,  _halfExtent,
         _halfExtent, -_halfExtent, -_halfExtent,
        // Face -Z
         _halfExtent,  _halfExtent, -_halfExtent,
         _halfExtent, -_halfExtent, -_halfExtent,
        -_halfExtent,  _halfExtent, -_halfExtent,
        -_halfExtent, -_halfExtent, -_halfExtent,
    };

    constexpr auto normals = std::array{
        // Face +X
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        // Face +Y
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        // Face +Z
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        // Face -X
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        // Face -Y
        0.0f, -1.0f, 0.0f,
        0.0f, -1.0f, 0.0f,
        0.0f, -1.0f, 0.0f,
        0.0f, -1.0f, 0.0f,
        // Face -Z
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,
    };

    constexpr auto texCoords = std::array{
        // Face +X
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        // Face +Y
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        // Face +Z
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        // Face -X
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        // Face -Y
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        // Face -Z
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
    };

    auto indices = std::vector<uint32_t>{};
    indices.reserve(36);
    for (auto i = 0; i < 6; ++i) {
        const auto it = i * 4;
        indices.push_back(it);     indices.push_back(it + 1); indices.push_back(it + 2);
        indices.push_back(it + 2); indices.push_back(it + 1); indices.push_back(it + 3);
    }

    auto geometry = Geometry::Builder()
        .vertexCount(positions.size() / 3)
        .indexCount(indices.size())
        .build(renderer.getResourceAllocator());

    geometry->setVertexData("position", positions.data(), renderer);
    geometry->setVertexData("normal",   normals.data(),   renderer);
    geometry->setVertexData("uv",       texCoords.data(), renderer);

    geometry->setIndexData(indices.data(), sizeof(uint32_t) * indices.size(), renderer);

    return geometry;
}
