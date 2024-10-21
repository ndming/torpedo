#include <torpedo/renderer/ForwardRenderer.h>
#include <torpedo/graphics/Material.h>
#include <torpedo/camera/PerspectiveCamera.h>

#include <GLFW/glfw3.h>

#include <plog/Init.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Formatters/TxtFormatter.h>

static constexpr auto positions = std::array{
     0.0f,  0.5f,  0.0f,  // 1st vertex
    -0.5f, -0.5f,  0.0f,  // 2nd vertex
     0.5f, -0.5f,  0.0f,  // 3rd vertex
};

static constexpr auto colors = std::array{
    1.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f, 1.0f,
};

static constexpr auto indices = std::array<uint32_t, 3>{ 0, 1, 2 };

static constexpr auto instances = std::array<uint16_t, 3>{ 0, 1, 2 };

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

    const auto renderer = tpd::createRenderer<tpd::ForwardRenderer>(window);

    const auto geometry = tpd::Geometry::Builder()
        .vertexCount(3)
        .indexCount(indices.size())
        .maxInstanceCount(3)
        .attributeCount(3)
        .vertexAttribute(0, vk::Format::eR32G32B32Sfloat,    sizeof(float) * 3)
        .vertexAttribute(1, vk::Format::eR32G32B32A32Sfloat, sizeof(float) * 4)
        .instanceAttribute(2, vk::Format::eR16Uint,          sizeof(uint16_t))
        .build(*renderer);

    geometry->setVertexData(0, positions.data(), sizeof(float)    *  9, *renderer);
    geometry->setVertexData(1, colors.data(),    sizeof(float)    * 12, *renderer);
    geometry->setVertexData(2, instances.data(), sizeof(uint16_t) *  3, *renderer);
    geometry->setIndexData(indices.data(), sizeof(uint32_t) * indices.size(), *renderer);

    const auto material = tpd::Material::Builder()
        .vertShader("assets/shaders/simple.vert.spv")
        .fragShader("assets/shaders/simple.frag.spv")
        .build(*renderer);

    const auto materialInstance = material->createInstance();

    const auto drawable = tpd::Drawable::Builder()
        .instanceCount(1)
        .build(geometry, materialInstance);

    const auto light = tpd::AmbientLight::Builder()
        .color(1.0f, 1.0f, 0.0f)
        .build();

    const auto camera = renderer->createCamera<tpd::PerspectiveCamera>();
    const auto view = renderer->createView();
    view->scene->insert(drawable);
    view->scene->insert(light);
    view->camera = camera;

    camera->lookAt({ 2.0f, -2.0f, 2.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f });

    renderer->setOnFramebufferResize([&view, &camera](const uint32_t width, const uint32_t height) {
        const auto aspect = static_cast<float>(width) / static_cast<float>(height);
        camera->setProjection(45.0f, aspect, 0.1f, 32.0f);
        view->setSize(width, height);
    });

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        renderer->render(*view);
    }
    renderer->waitIdle();

    glfwDestroyWindow(window);
    glfwTerminate();
}
