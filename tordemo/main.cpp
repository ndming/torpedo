#include <torpedo/renderer/ForwardRenderer.h>
#include <torpedo/graphics/Material.h>
#include <torpedo/camera/PerspectiveCamera.h>
#include <torpedo/geometry/CubeGeometryBuilder.h>

#include <GLFW/glfw3.h>

#include <plog/Init.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Formatters/TxtFormatter.h>

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
    const auto geometry = tpd::CubeGeometryBuilder().build(*renderer);

    const auto material = tpd::Material::Builder()
        .vertShader("assets/shaders/simple.vert.spv")
        .fragShader("assets/shaders/simple.frag.spv")
        .build(*renderer);

    const auto materialInstance = material->createInstance();

    const auto drawable = tpd::Drawable::Builder().build(geometry, materialInstance);

    const auto light = tpd::PointLight::Builder()
        .position(2.0f, 2.0f, 2.0f)
        .intensity(10.0f)
        .build();

    const auto camera = renderer->createCamera<tpd::PerspectiveCamera>();
    const auto view = renderer->createView();
    view->scene->insert(drawable);
    view->scene->insert(light);
    view->camera = camera;

    camera->lookAt({ 4.0f, 4.0f, 4.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f });

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
