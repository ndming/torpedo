#include <torpedo/rendering/Context.h>
#include <torpedo/rendering/SurfaceRenderer.h>
#include <torpedo/volumetric/GaussianEngine.h>
#include <torpedo/utility/Log.h>

int main() {
    tpd::utils::plantConsoleLogger();
    const auto context = tpd::Context<tpd::SurfaceRenderer>::create();

    const auto renderer = context->initRenderer(1280, 720);
    renderer->getWindow()->setTitle("Hello, Gaussian!");

    const auto engine = context->bindEngine<tpd::GaussianEngine>();

    renderer->getWindow()->loop([&] {

    });
    engine->waitIdle();
}
