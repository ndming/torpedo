#include <torpedo/rendering/Context.h>
#include <torpedo/rendering/SurfaceRenderer.h>
#include <torpedo/rendering/LogUtils.h>
#include <torpedo/volumetric/GaussianEngine.h>

#include <exception>

int main() {
    try {
        tpd::utils::plantConsoleLogger();
        const auto context = tpd::Context<tpd::SurfaceRenderer>::create();

        const auto renderer = context->initRenderer(1280, 720);
        renderer->getWindow()->setTitle("Hello, Gaussian!");

        const auto engine = context->bindEngine<tpd::GaussianEngine>();

        renderer->getWindow()->loop([&] {
            engine->preFrameCompute();
            if (const auto swapImage = renderer->launchFrame(); swapImage) {
                engine->draw(swapImage);
                renderer->submitFrame(swapImage.index);
            }
        });
        engine->waitIdle();
    } catch (const std::exception& e) {
        tpd::utils::logError(e.what());
    }
}
