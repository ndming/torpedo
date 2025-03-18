#include <torpedo/rendering/StandardRenderer.h>
#include <torpedo/rendering/ForwardEngine.h>
#include <torpedo/utils/Log.h>

#include <torpedo/foundation/StorageBuffer.h>

#include <exception>

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

    try {
        auto renderer = tpd::StandardRenderer{};
        renderer.init(1280, 720);
        renderer.getContext().setWindowTitle("Hello, Triangle!");

        auto engine = tpd::ForwardEngine{};
        engine.init(renderer);

        auto storage = tpd::StorageBuffer::Builder()
            .alloc(sizeof(float) * positions.size())
            .build(engine.getDeviceAllocator());
        storage.setSyncData(positions.data(), sizeof(float) * positions.size());
        engine.sync(storage);

        renderer.getContext().loop([&] {
            if (const auto [valid, image, imageIndex] = renderer.beginFrame(); valid) {
                const auto buffer = engine.draw(image);
                renderer.endFrame(buffer, imageIndex);
            }
        });
    } catch (const std::exception& e) {
        tpd::utils::logError(e.what());
    }

    return 0;
}
