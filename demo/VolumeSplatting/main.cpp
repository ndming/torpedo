#include <torpedo/rendering/Context.h>
#include <torpedo/rendering/Scene.h>
#include <torpedo/rendering/SurfaceRenderer.h>
#include <torpedo/rendering/LogUtils.h>

#include <torpedo/volumetric/GaussianEngine.h>
#include <torpedo/volumetric/GaussianGeometry.h>

#include <torpedo/extension/OrbitControl.h>
#include <torpedo/extension/PerspectiveCamera.h>

#include <filesystem>

int main() {
    tpd::utils::plantConsoleLogger();
    const auto context = tpd::Context<tpd::SurfaceRenderer>::create();

    const auto renderer = context->initRenderer(1280, 720);
    renderer->getWindow()->setTitle("Volume Splatting");

    const auto engine = context->bindEngine<tpd::GaussianEngine>();
    const auto camera = context->createCamera<tpd::PerspectiveCamera>();

    // Mouse left: orbit, mouse right: pan, mouse wheel: zoom
    const auto control = renderer->getWindow()->createControl<tpd::OrbitControl>();
    control->setSensitivity(.5f);
    control->setRadius(110.f);

    // CMake has downloaded a trained point cloud
    const auto plyFile = std::filesystem::path(VOLUME_SPLATTING_ASSETS_DIR) / "counter-iter-30000.ply";
    auto points = tpd::GaussianPoint::fromModel(plyFile);

    auto scene = tpd::Scene{};
    scene.add(tpd::group(points));

    auto settings = tpd::GaussianEngine::Settings::getDefault();
    settings.shDegree = 3;

    engine->compile(scene, settings);
    points.clear();

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
