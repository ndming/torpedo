#include <torpedo/graphics/Context.h>
#include <torpedo/geometry/CubeGeometryBuilder.h>
#include <torpedo/material/PhongMaterial.h>
#include <torpedo/renderer/ForwardRenderer.h>

#include <torpedo/camera/PerspectiveCamera.h>
#include <torpedo/control/OrbitControl.h>

int main() {
    const auto context = tpd::Context::create("Phong Cube");
    const auto renderer = tpd::createRenderer<tpd::ForwardRenderer>(*context);

    const auto geometry = tpd::CubeGeometryBuilder().build(*renderer);
    const auto material = tpd::PhongMaterial::Builder().build(*renderer);
    const auto materialInstance = material->createInstance(*renderer);

    // Gold-like Phong surface
    materialInstance->setDiffuse(0.75164f, 0.60648f, 0.22648f);
    materialInstance->setSpecular(0.628281f, 0.555802f, 0.366065f);
    materialInstance->setShininess(51.2f);

    const auto cube = tpd::Drawable::Builder().build(geometry, materialInstance);

    const auto pointLight = tpd::PointLight::Builder()
        .position(2.0f, 2.0f, 2.0f)
        .decay(0.5f)
        .intensity(10.0f)
        .build();

    const auto ambientLight = tpd::AmbientLight::Builder()
        .intensity(0.02f)
        .build();

    const auto view = renderer->createView();
    view->scene->insert(cube);
    view->scene->insert(pointLight);
    view->scene->insert(ambientLight);

    const auto camera = renderer->createCamera<tpd::PerspectiveCamera>();
    view->camera = camera;

    const auto control = tpd::OrbitControl::Builder()
        .initialRadius(6.0f)
        .build(*context);

    renderer->setOnFramebufferResize([&view, &camera](const uint32_t width, const uint32_t height) {
        view->setSize(width, height);

        const auto aspect = static_cast<float>(width) / static_cast<float>(height);
        camera->setProjection(45.0f, aspect, 0.1f, 200.0f);
    });

    context->loop([&](const float deltaTimeMillis) {
        renderer->render(*view);
        control->update(*view, deltaTimeMillis);
    });
    renderer->waitIdle();
}
