#include <torpedo/graphics/Context.h>
#include <torpedo/graphics/Material.h>
#include <torpedo/renderer/ForwardRenderer.h>

static constexpr auto positions = std::array{
    0.0f, -0.5f,  0.0f,  // 1st vertex
   -0.5f,  0.5f,  0.0f,  // 2nd vertex
    0.5f,  0.5f,  0.0f,  // 3rd vertex
};

static constexpr auto colors = std::array{
    1.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f, 1.0f,
};

static constexpr auto indices = std::array<uint16_t, 3>{ 0, 1, 2 };

int main() {
    const auto context = tpd::Context::create("Hello, Triangle!");
    const auto renderer = tpd::createRenderer<tpd::ForwardRenderer>(*context);

    const auto geometry = tpd::Geometry::Builder()
        .vertexCount(3)
        .indexCount(3)
        .indexType(vk::IndexType::eUint16)
        .attributeCount(2)
        .vertexAttribute(0, vk::Format::eR32G32B32Sfloat,    sizeof(float) * 3)
        .vertexAttribute(1, vk::Format::eR32G32B32A32Sfloat, sizeof(float) * 4)
        .build(*renderer);

    geometry->setVertexData(0, positions.data(), *renderer);
    geometry->setVertexData(1, colors.data(),    *renderer);

    // When not specifying size, Geometry assumes uint32_t indices by default
    geometry->setIndexData(indices.data(), sizeof(uint16_t) * 3, *renderer);

    const auto material = tpd::Material::Builder()
        .vertShader("shaders/simple.vert.spv")
        .fragShader("shaders/simple.frag.spv")
        .build(*renderer);
    const auto materialInstance = material->createInstance();

    const auto triangle = tpd::Drawable::Builder().build(geometry, materialInstance);

    const auto view = renderer->createView();
    view->scene->insert(triangle);

    renderer->setOnFramebufferResize([&view](const uint32_t width, const uint32_t height) {
        view->setSize(width, height);
    });

    context->loop([&] {
        renderer->render(*view);
    });
    renderer->waitIdle();
}
