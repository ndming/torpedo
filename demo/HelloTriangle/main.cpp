#include <torpedo/rendering/Context.h>
#include <torpedo/rendering/SurfaceRenderer.h>
#include <torpedo/rendering/ForwardEngine.h>
#include <torpedo/utility/Log.h>

static constexpr auto positions = std::array{
    0.0f, -0.5f,  0.0f,  // 1st vertex
   -0.5f,  0.5f,  0.0f,  // 2nd vertex
    0.5f,  0.5f,  0.0f,  // 3rd vertex
};

static constexpr auto colors = std::array{
    1.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f, 1.0f,
};

static constexpr auto indices = std::array<uint16_t, 3>{ 0, 1, 2 };

int main() {
    tpd::utils::plantConsoleLogger();
    const auto context = tpd::Context<tpd::SurfaceRenderer>::create();
    const auto engine = context->bindEngine<tpd::ForwardEngine>();

    const auto renderer = context->initRenderer(1280, 720);
    renderer->getWindow()->setTitle("Hello, Triangle");

    renderer->getWindow()->loop([&] {
        if (const auto [valid, image, imageIndex] = renderer->onFrame(); valid) {
            const auto [primaryBuffer, doneStage] = engine->draw(image);
            renderer->submitFrame(primaryBuffer, doneStage, imageIndex);
        }
    });
    engine->waitIdle();

    return 0;
}
