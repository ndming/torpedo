#include <torpedo/rendering/Context.h>
#include <torpedo/rendering/SurfaceRenderer.h>
#include <torpedo/rendering/LogUtils.h>

#include <torpedo/volumetric/GaussianEngine.h>

#include <torpedo/extension/OrbitControl.h>
#include <torpedo/extension/PerspectiveCamera.h>

int main() {
    tpd::utils::plantConsoleLogger();
    const auto context = tpd::Context<tpd::SurfaceRenderer>::create();

    const auto renderer = context->initRenderer(1280, 720);
    renderer->getWindow()->setTitle("Hello, Gaussian!");

    const auto engine = context->bindEngine<tpd::GaussianEngine>();
    const auto camera = context->createCamera<tpd::PerspectiveCamera>();

    const auto control = renderer->getWindow()->createControl<tpd::OrbitControl>();
    control->setSensitivity(.5f);
    control->setRadius(8.f);

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
