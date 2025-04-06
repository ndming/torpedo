#include <torpedo/rendering/Context.h>
#include <torpedo/rendering/SurfaceRenderer.h>
#include <torpedo/rendering/LogUtils.h>
#include <torpedo/volumetric/GaussianEngine.h>

#include <exception>

int main() {
    try {
        tpd::rendering::plantConsoleLogger();
        const auto context = tpd::Context<tpd::SurfaceRenderer>::create();

        const auto renderer = context->initRenderer(1280, 720);
        renderer->getWindow()->setTitle("Hello, Gaussian!");

        const auto engine = context->bindEngine<tpd::GaussianEngine>();

        renderer->getWindow()->loop([&] {
            engine->preFrameCompute();
            if (const auto [valid, image, imageIndex] = renderer->launchFrame(); valid) {
                const auto [buffer, waitStage, doneStage, waits] = engine->draw(image);
                renderer->submitFrame(imageIndex, buffer, waitStage, doneStage, waits);
            }
        });
        engine->waitIdle();
    } catch (const std::exception& e) {
        tpd::rendering::logError(e.what());
    }
}
