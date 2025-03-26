#include <torpedo/rendering/Context.h>
#include <torpedo/rendering/SurfaceRenderer.h>
#include <torpedo/volumetric/GaussianEngine.h>
#include <torpedo/utility/Log.h>

#include <exception>

int main() {
    try {
        tpd::utils::plantConsoleLogger();
        const auto context = tpd::Context<tpd::SurfaceRenderer>::create();
    
        const auto renderer = context->initRenderer(1280, 720);
        renderer->getWindow()->setTitle("Hello, Gaussian!");
    
        auto engine = context->bindEngine<tpd::GaussianEngine>();
    
        renderer->getWindow()->loop([&] {
            if (const auto [valid, image, imageIndex] = renderer->launchFrame(); valid) {
                const auto [buffer, waitStage, doneStage] = engine->draw(image);
                renderer->submitFrame(buffer, waitStage, doneStage, imageIndex);
            }
        });
        engine->waitIdle();
    } catch (const std::exception& e) {
        tpd::utils::logError(e.what());
    }
}
