#pragma once

#include "TordemoApplication.h"

#include <torpedo/foundation/Buffer.h>
#include <torpedo/foundation/PipelineShader.h>
#include <torpedo/foundation/ShaderLayout.h>

class HelloTriangle final : public TordemoApplication {
public:
    HelloTriangle() = default;
    ~HelloTriangle() override;

    HelloTriangle(const HelloTriangle&) = delete;
    HelloTriangle& operator=(const HelloTriangle&) = delete;

private:
    [[nodiscard]] std::string getWindowTitle() const override { return "Hello, Triangle!"; }

    void onInitialized() override;

    void createPipeline();
    static vk::PipelineVertexInputStateCreateInfo getVertexInputState();
    [[nodiscard]] vk::PipelineMultisampleStateCreateInfo getMultisampleState() const;
    vk::PipelineLayout _pipelineLayout{};
    vk::Pipeline _graphicsPipeline{};

    void createBuffers();
    std::unique_ptr<tpd::Buffer> _vertexBuffer{};
    std::unique_ptr<tpd::Buffer> _indexBuffer{};

    void onDraw(vk::CommandBuffer buffer) override;
};
