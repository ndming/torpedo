#include <torpedo/renderer/ForwardRenderer.h>
#include <torpedo/material/PhongMaterial.h>
#include <torpedo/camera/PerspectiveCamera.h>
#include <torpedo/geometry/CubeGeometryBuilder.h>

#include <GLFW/glfw3.h>

#include <plog/Init.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Formatters/TxtFormatter.h>

#include "stb.h"

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
    const auto phong = tpd::PhongMaterial::Builder().build(*renderer);

    int texWidth, texHeight, texChannels;
    auto pixels = stbi_load("assets/textures/container_diffuse.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    const auto diffuse = tpd::Texture::Builder()
        .size(texWidth, texHeight)
        .build(*renderer);
    diffuse->setImageData(pixels, texWidth, texHeight, *renderer);

    stbi_image_free(pixels);

    pixels = stbi_load("assets/textures/container_specular.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    const auto specular = tpd::Texture::Builder()
        .size(texWidth, texHeight)
        .build(*renderer);
    specular->setImageData(pixels, texWidth, texHeight, *renderer);

    stbi_image_free(pixels);

    const auto phongInstance = phong->createInstance(*renderer);
    phongInstance->setDiffuse(*diffuse);
    phongInstance->setSpecular(*specular);

    const auto drawable = tpd::Drawable::Builder().build(geometry, phongInstance);

    const auto light = tpd::PointLight::Builder()
        .position(-2.0f, -2.0f, 2.0f)
        .decay(0.5f)
        .intensity(10.0f)
        .build();

    const auto ambientLight = tpd::AmbientLight::Builder()
        .intensity(0.02f)
        .build();

    const auto camera = renderer->createCamera<tpd::PerspectiveCamera>();
    const auto view = renderer->createView();
    view->scene->insert(drawable);
    view->scene->insert(light);
    view->scene->insert(ambientLight);
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
