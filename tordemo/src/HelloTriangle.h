#pragma once

#include "TordemoApplication.h"

#include <torpedo/foundation/Buffer.h>
#include <torpedo/foundation/PipelineShader.h>

class HelloTriangle final : public TordemoApplication {
public:
    HelloTriangle() = default;
    ~HelloTriangle() override;

    HelloTriangle(const HelloTriangle&) = delete;
    HelloTriangle& operator=(const HelloTriangle&) = delete;

private:
    [[nodiscard]] std::string getWindowTitle() const override { return "Hello, Triangle!"; }

    void createPipelineResources() override;
    void createBuffers();
    std::shared_ptr<tpd::Buffer> _vertexBuffer{};
    std::shared_ptr<tpd::Buffer> _indexBuffer{};

    [[nodiscard]] vk::PipelineVertexInputStateCreateInfo getGraphicsPipelineVertexInputState() const override;
    std::unique_ptr<tpd::PipelineShader> buildPipelineShader(vk::GraphicsPipelineCreateInfo* pipelineInfo) const override;

    void render(vk::CommandBuffer buffer) override;
};
