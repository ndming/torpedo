#include <torpedo/graphics/Engine.h>
#include <torpedo/graphics/Geometry.h>
#include <torpedo/graphics/Material.h>
#include <GLFW/glfw3.h>

#include <plog/Init.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Formatters/TxtFormatter.h>

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

static constexpr auto indices = std::array<uint32_t, 3>{ 0, 1, 2 };

int main() {
    auto appender = plog::ColorConsoleAppender<plog::TxtFormatter>();
#ifdef NDEBUG
    init(plog::info, &appender);
#else
    init(plog::debug, &appender);
#endif

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    const auto window = glfwCreateWindow(1280, 768, "Hello, Triangle!", nullptr, nullptr);

    const auto engine = tpd::Engine::create();
    const auto renderer = engine->createRenderer(tpd::RenderEngine::Forward, window);

    const auto geometry = tpd::Geometry::Builder(3, indices.size())
        .attributeCount(2)
        .vertexAttribute(0, vk::Format::eR32G32B32Sfloat,    sizeof(float) * 3)
        .vertexAttribute(1, vk::Format::eR32G32B32A32Sfloat, sizeof(float) * 4)
        .build(*engine);

    geometry->setVertexData(0, positions.data(), sizeof(float) *  9, *engine);
    geometry->setVertexData(1, colors.data(),    sizeof(float) * 12, *engine);
    geometry->setIndexData(indices.data(), sizeof(uint32_t) * indices.size(), *engine);

    const auto material = tpd::Material::Builder()
        .vertShader("assets/shaders/simple.vert.spv")
        .fragShader("assets/shaders/simple.frag.spv")
        .build(*renderer);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
    renderer->waitIdle();

    material->dispose(*renderer);
    geometry->dispose(*engine);
    engine->destroyRenderer(renderer);

    glfwDestroyWindow(window);
    glfwTerminate();
}
