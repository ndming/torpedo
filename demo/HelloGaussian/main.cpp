#include <torpedo/rendering/Context.h>
#include <torpedo/rendering/Scene.h>
#include <torpedo/rendering/SurfaceRenderer.h>
#include <torpedo/rendering/LogUtils.h>

#include <torpedo/volumetric/GaussianEngine.h>
#include <torpedo/volumetric/GaussianGeometry.h>

#include <torpedo/extension/OrbitControl.h>
#include <torpedo/extension/PerspectiveCamera.h>

int main() {
    tpd::utils::plantConsoleLogger();
    const auto context = tpd::Context<tpd::SurfaceRenderer>::create();

    const auto renderer = context->initRenderer(1280, 720);
    renderer->getWindow()->setTitle("Hello, Gaussian!");

    const auto engine = context->bindEngine<tpd::GaussianEngine>();
    const auto camera = context->createCamera<tpd::PerspectiveCamera>();

    // Mouse left: orbit, mouse right: pan, mouse wheel: zoom
    const auto control = renderer->getWindow()->createControl<tpd::OrbitControl>();
    control->setSensitivity(.5f);
    control->setRadius(8.f);

    // Generate random points within a cube of size 10 centered at the origin
    auto points = tpd::GaussianPoint::random(8192, 10.f, {}, 0.005f, 0.2f);

    // A big, white, uniform Gaussian at the center of the scene
    auto gaussian = tpd::GaussianPoint{
        .position = { 0.f, 0.f, 0.f },
        .opacity = 1.f,
        .quaternion = { 0.f, 0.f, 0.f, 1.f },
        .scale = { 2.f, 2.f, 2.f, 1.0f },
        .sh = tpd::utils::rgb2sh(1.0f, 1.0f, 1.0f),
    };

    auto scene = tpd::Scene{};
    scene.add(tpd::ent::group(points));
    scene.add(std::move(gaussian));

    auto settings = tpd::GaussianEngine::Settings::getDefault();
    settings.shDegree = 0;

    engine->compile(scene, settings);
    points.clear(); // all data has been transferred to the GPU

    renderer->loop([&](const float deltaTimeMillis) {
        const auto [eye, tar] = control->getCameraUpdate(deltaTimeMillis);
        camera->lookAt(eye, tar, tpd::OrbitControl::getCameraUp());

        engine->preFrameCompute(*camera);
        if (const auto swapImage = renderer->launchFrame(); swapImage) {
            engine->draw(swapImage);
            renderer->submitFrame(swapImage.index);
        }
    });

    return 0;
}
