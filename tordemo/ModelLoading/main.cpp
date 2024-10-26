#include <torpedo/graphics/Context.h>
#include <torpedo/renderer/ForwardRenderer.h>

#include <torpedo/camera/PerspectiveCamera.h>
#include <torpedo/control/OrbitControl.h>

#include "ModelLoader.h"

int main() {
    const auto context = tpd::Context::create("Model Loading");
    const auto renderer = tpd::createRenderer<tpd::ForwardRenderer>(*context);

    const auto modelLoader = ModelLoader{ "models/backpack/backpack.obj", *renderer };
    modelLoader.drawable->setTransform(rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));

    const auto view = renderer->createView();
    view->scene->insert(modelLoader.drawable);

    const auto light = tpd::PointLight::Builder()
        .position(-2.0f, -2.0f, 2.0f)
        .decay(0.5f)
        .intensity(10.0f)
        .build();

    const auto ambientLight = tpd::AmbientLight::Builder()
        .intensity(0.02f)
        .build();

    view->scene->insert(light);
    view->scene->insert(ambientLight);

    const auto camera = renderer->createCamera<tpd::PerspectiveCamera>();
    view->camera = camera;

    const auto control = tpd::OrbitControl::Builder()
        .initialRadius(6.0f)
        .sensitivity(0.8f)
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
