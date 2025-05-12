#include <torpedo/rendering/Context.h>
#include <torpedo/rendering/SurfaceRenderer.h>
#include <torpedo/rendering/LogUtils.h>
#include <torpedo/volumetric/GaussianEngine.h>

#include <torpedo/extension/PerspectiveCamera.h>

int main() {
    tpd::utils::plantConsoleLogger();
    const auto context = tpd::Context<tpd::SurfaceRenderer>::create();

    const auto renderer = context->initRenderer(1280, 720);
    renderer->getWindow()->setTitle("Hello, Gaussian!");

    const auto engine = context->bindEngine<tpd::GaussianEngine>();

    const auto camera = context->createCamera<tpd::PerspectiveCamera>();
    camera->lookAt({ 0.f, 0.f, -6.f }, { 0.f, 0.f, 0.f }, { 0.f, -1.f, 0.f });

    renderer->loop([&] {
        engine->preFrameCompute(*camera);
        if (const auto swapImage = renderer->launchFrame(); swapImage) {
            engine->draw(swapImage);
            renderer->submitFrame(swapImage.index);
        }
    });

    return 0;
}
