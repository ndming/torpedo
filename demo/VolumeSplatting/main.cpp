#include <torpedo/rendering/Context.h>
#include <torpedo/rendering/Scene.h>
#include <torpedo/rendering/SurfaceRenderer.h>
#include <torpedo/rendering/LogUtils.h>

#include <torpedo/volumetric/GaussianEngine.h>
#include <torpedo/volumetric/GaussianGeometry.h>

#include <torpedo/extension/OrbitControl.h>
#include <torpedo/extension/PerspectiveCamera.h>

#include <filesystem>

static constexpr auto TRANSFORM = tpd::mat4{ 
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, -1.0f,
    0.0f, 0.0f, 1.0f, -0.5f,
    0.0f, 0.0f, 0.0f, 1.0f,
};

int main() {
    tpd::utils::plantConsoleLogger();
    const auto context = tpd::Context<tpd::SurfaceRenderer>::create();

    const auto renderer = context->initRenderer(1280, 720);
    renderer->getWindow()->setTitle("Volume Splatting");

    // CMake has downloaded a trained point cloud
    const auto plyFile = std::filesystem::path(VOLUME_SPLATTING_ASSETS_DIR) / "bicycle-iter-30000.ply";
    const auto points = tpd::GaussianPoint::fromModel(plyFile);

    auto scene = tpd::Scene{};
    const auto pointCloud = scene.add(tpd::ent::group(points));

    auto settings = tpd::GaussianEngine::Settings::getDefault();
    settings.shDegree = 0;

    const auto engine = context->bindEngine<tpd::GaussianEngine>();
    engine->compile(scene, settings);

    const auto& transformHost = engine->getTransformHost();
    transformHost->transform(pointCloud, TRANSFORM);

    const auto camera = context->createCamera<tpd::PerspectiveCamera>();
    camera->lookAt({ -2.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f });

    renderer->loop([&] {
        engine->preFrameCompute(*camera);
        if (const auto swapImage = renderer->launchFrame(); swapImage) {
            engine->draw(swapImage);
            renderer->submitFrame(swapImage.index);
        }
    });

    return 0;
}
