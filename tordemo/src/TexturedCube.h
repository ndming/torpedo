#pragma once

#include "TordemoApplication.h"

#include <torpedo/foundation/Buffer.h>
#include <torpedo/foundation/Image.h>
#include <torpedo/foundation/PipelineInstance.h>

#include <glm/glm.hpp>

class TexturedCube final : public TordemoApplication {
public:
    TexturedCube() = default;
    ~TexturedCube() override;

    TexturedCube(const TexturedCube &) = delete;
    TexturedCube& operator=(const TexturedCube&) = delete;

private:
    [[nodiscard]] std::string getWindowTitle() const override { return "Textured Cube"; }

    void createPipelineResources() override;

    void createDrawingBuffers();
    std::shared_ptr<tpd::Buffer> _vertexBuffer{};
    std::shared_ptr<tpd::Buffer> _indexBuffer{};
    std::vector<uint16_t> _indices{};

    struct MVP {
        glm::mat4 model{ 1.0f };
        glm::mat4 view{ 1.0f };
        glm::mat4 proj{ 1.0f };
    };
    void createUniformBuffer();
    std::shared_ptr<tpd::Buffer> _uniformBuffer{};

    void createTextureResources();
    std::unique_ptr<tpd::Image> _texture{};
    vk::Sampler _sampler{};

    [[nodiscard]] vk::PipelineVertexInputStateCreateInfo getGraphicsPipelineVertexInputState() const override;
    std::unique_ptr<tpd::PipelineShader> buildPipelineShader(vk::GraphicsPipelineCreateInfo* pipelineInfo) const override;

    void createPipelineInstance();
    std::unique_ptr<tpd::PipelineInstance> _pipelineInstance{};

    void onFrameReady() override;
    void render(vk::CommandBuffer buffer) override;
};
