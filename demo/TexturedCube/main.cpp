#include <torpedo/graphics/Context.h>
#include <torpedo/geometry/CubeGeometryBuilder.h>
#include <torpedo/material/PhongMaterial.h>
#include <torpedo/renderer/ForwardRenderer.h>

#include <torpedo/camera/PerspectiveCamera.h>
#include <torpedo/control/OrbitControl.h>

#include "stb.h"

int main() {
    const auto context = tpd::Context::create("Hello, Container!");
    const auto renderer = tpd::createRenderer<tpd::ForwardRenderer>(*context);

    const auto geometry = tpd::CubeGeometryBuilder().build(*renderer);
    const auto phong = tpd::PhongMaterial::Builder().build(*renderer);

    int texWidth, texHeight, texChannels;
    auto pixels = stbi_load("textures/diffuse.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    const auto diffuse = tpd::Texture::Builder()
        .size(texWidth, texHeight)
        .build(*renderer);
    diffuse->setImageData(pixels, texWidth, texHeight, *renderer);

    stbi_image_free(pixels);

    pixels = stbi_load("textures/specular.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    const auto specular = tpd::Texture::Builder()
        .size(texWidth, texHeight)
        .build(*renderer);
    specular->setImageData(pixels, texWidth, texHeight, *renderer);

    stbi_image_free(pixels);

    const auto phongInstance = phong->createInstance(*renderer);
    phongInstance->setDiffuse(*diffuse);
    phongInstance->setSpecular(*specular);

    const auto drawable = tpd::Drawable::Builder()
        .meshCount(1)
        .mesh(0, geometry, phongInstance)
        .build();

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

    const auto control = tpd::OrbitControl::Builder()
        .initialRadius(6.0f)
        .build(*context);

    renderer->setOnFramebufferResize([&view, &camera](const uint32_t width, const uint32_t height) {
        const auto aspect = static_cast<float>(width) / static_cast<float>(height);
        camera->setProjection(45.0f, aspect, 0.1f, 200.0f);
        view->setSize(width, height);
    });

    context->loop([&](const float deltaTimeMillis) {
        renderer->render(*view);
        control->update(*view, deltaTimeMillis);
    });
    renderer->waitIdle();
}
