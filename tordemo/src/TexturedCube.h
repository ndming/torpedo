#pragma once

#include "TordemoApplication.h"

#include <torpedo/foundation/Buffer.h>
#include <torpedo/foundation/Image.h>
#include <torpedo/foundation/ShaderLayout.h>
#include <torpedo/foundation/ShaderInstance.h>

class TexturedCube final : public TordemoApplication {
public:
    TexturedCube() = default;
    ~TexturedCube() override;

    TexturedCube(const TexturedCube &) = delete;
    TexturedCube& operator=(const TexturedCube&) = delete;

private:
    [[nodiscard]] std::string getWindowTitle() const override { return "Textured Cube"; }

    void onInitialized() override;

    void createPipeline();
    static vk::PipelineVertexInputStateCreateInfo getVertexInputState();
    [[nodiscard]] vk::PipelineMultisampleStateCreateInfo getMultisampleState() const;
    std::unique_ptr<tpd::ShaderLayout> _shaderLayout{};
    vk::Pipeline _graphicsPipeline{};

    struct Vertex {
        std::array<float, 3> position;
        std::array<float, 4> color;
        std::array<float, 2> texCoord;
    };
    void createDrawingBuffers();
    std::unique_ptr<tpd::Buffer> _vertexBuffer{};
    std::unique_ptr<tpd::Buffer> _indexBuffer{};
    std::vector<uint16_t> _indices{};

    struct MVP {
        std::array<float, 16> model{ 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
        std::array<float, 16> view { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
        std::array<float, 16> proj { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
    };
    void createUniformBuffer();
    std::unique_ptr<tpd::Buffer> _uniformBuffer{};

    void createTextureResources();
    std::unique_ptr<tpd::Image> _texture{};
    vk::Sampler _sampler{};

    void createShaderInstance();
    std::unique_ptr<tpd::ShaderInstance> _shaderInstance{};

    void onFrameReady() override;
    void onDraw(vk::CommandBuffer buffer) override;
};
