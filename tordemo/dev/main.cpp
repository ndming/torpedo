#include <torpedo/graphics/Context.h>
#include <torpedo/renderer/ForwardRenderer.h>
#include <torpedo/material/PhongMaterial.h>
#include <torpedo/camera/PerspectiveCamera.h>
#include <torpedo/control/OrbitControl.h>

#include "stb.h"

static constexpr auto positions = std::array{
    -1.0f, -1.0f,  0.0f,
     1.0f, -1.0f,  0.0f,
    -1.0f,  1.0f,  0.0f,
     1.0f,  1.0f,  0.0f,
};

static constexpr auto normals = std::array{
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,
};

static constexpr auto uvs = std::array{
    0.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 0.0f,
    1.0f, 1.0f,
};

static constexpr auto indices = std::array<uint16_t, 4>{ 0, 1, 2, 3 };

std::shared_ptr<tpd::Texture> loadTexture(const std::filesystem::path& path, const tpd::Renderer& renderer);

int main() {
    const auto context = tpd::Context::create("dev");
    const auto renderer = tpd::createRenderer<tpd::ForwardRenderer>(*context);

    const auto geometry = tpd::Geometry::Builder()
        .vertexCount(positions.size() / 3)
        .indexCount(indices.size())
        .indexType(vk::IndexType::eUint16)
        .build(*renderer, vk::PrimitiveTopology::eTriangleStrip);
    geometry->setVertexData("position", positions.data(), *renderer);
    geometry->setVertexData("normal",   normals.data(),   *renderer);
    geometry->setVertexData("uv",       uvs.data(),       *renderer);
    geometry->setIndexData(indices.data(), sizeof(uint16_t) * indices.size(), *renderer);

    const auto material = tpd::PhongMaterial::Builder().build(*renderer);
    const auto materialInstance = material->createInstance(*renderer);

    const auto floorDiffuseTexture = loadTexture("textures/parquet/diffuse.png", *renderer);
    materialInstance->setDiffuse(*floorDiffuseTexture);
    materialInstance->setShininess(2.0f);

    const auto floor = tpd::Drawable::Builder()
        .meshCount(1)
        .mesh(0, geometry, materialInstance)
        .build();
    floor->setTransform(scale(glm::mat4(1.0f), glm::vec3(20.0f, 20.0f, 20.0f)));

    const auto view = renderer->createView();
    view->scene->insert(floor);

    const auto light = tpd::PointLight::Builder()
        .position(0.0f, 5.0f, 1.0f)
        .decay(0.5f)
        .intensity(10.0f)
        .build();

    const auto ambientLight = tpd::AmbientLight::Builder()
        .intensity(0.02f)
        .build();

    view->scene->insert(light);
    view->scene->insert(ambientLight);

    const auto camera = renderer->createCamera<tpd::PerspectiveCamera>();
    view->camera = camera;

    const auto control = tpd::OrbitControl::Builder()
        .initialRadius(6.0f)
        .build(*context);

    renderer->setOnFramebufferResize([&view, &camera](const uint32_t width, const uint32_t height) {
        const auto aspect = static_cast<float>(width) / static_cast<float>(height);
        camera->setProjection(45.0f, aspect, 0.1f, 400.0f);
        view->setSize(width, height);
    });

    context->loop([&](const float deltaTimeMillis) {
        renderer->render(*view);
        control->update(*view, deltaTimeMillis);
    });
    renderer->waitIdle();
}

std::shared_ptr<tpd::Texture> loadTexture(const std::filesystem::path& path, const tpd::Renderer& renderer) {
    int texWidth, texHeight, texChannels;
    const auto pixels = stbi_load(path.string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    auto texture = tpd::Texture::Builder()
        .size(texWidth, texHeight)
        .build(renderer);
    texture->setImageData(pixels, texWidth, texHeight, renderer);

    stbi_image_free(pixels);

    return texture;
}
