#pragma once

#include "torpedo/graphics/Renderer.h"
#include "torpedo/graphics/MaterialInstance.h"

#include <torpedo/foundation/ShaderLayout.h>

#include <filesystem>

namespace tpd {
    class Material;

    template<typename T>
    concept Shadable = std::derived_from<T, Material>;

    class Material {
    public:
        class Builder {
        public:
            Builder& vertShader(const std::filesystem::path& path, std::string entryPoint = "main");
            Builder& fragShader(const std::filesystem::path& path, std::string entryPoint = "main");

            Builder& withCustomLayout();
            Builder& minSampleShading(float fraction);

            template<Shadable S = Material>
            [[nodiscard]] std::unique_ptr<S> build(const Renderer& renderer, std::unique_ptr<ShaderLayout> layout = {}) const;

        private:
            static std::vector<std::byte> readShaderFile(const std::filesystem::path& path);
            static vk::ShaderModule createShaderModule(vk::Device device, const std::vector<std::byte>& code);

            std::vector<std::byte> _vertShaderCode{};
            std::vector<std::byte> _fragShaderCode{};

            std::string _vertShaderEntryPoint{};
            std::string _fragShaderEntryPoint{};

            bool _customLayout{ false };
            float _minSampleShading{ 0.0f };
        };

        Material(vk::Device device, vk::Pipeline pipeline, std::unique_ptr<ShaderLayout> shaderLayout, bool withSharedLayout = true) noexcept;

        Material(const Material&) = delete;
        Material& operator=(const Material&) = delete;

        [[nodiscard]] std::shared_ptr<MaterialInstance> createInstance() const;

        [[nodiscard]] vk::Device getVulkanDevice() const;
        [[nodiscard]] vk::Pipeline getVulkanPipeline() const;
        [[nodiscard]] vk::PipelineLayout getVulkanPipelineLayout() const;
        [[nodiscard]] bool builtWithSharedLayout() const;

        static ShaderLayout::Builder getSharedLayoutBuilder(uint32_t additionalSetCount = 0);

        void dispose() noexcept;

        virtual ~Material();

    protected:
        static std::filesystem::path getShaderFilePath(std::string_view file);

        vk::Device _device;
        vk::Pipeline _pipeline;
        std::unique_ptr<ShaderLayout> _shaderLayout;
        bool _withSharedLayout;
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::Material::Builder& tpd::Material::Builder::vertShader(const std::filesystem::path& path, std::string entryPoint) {
    _vertShaderCode = readShaderFile(path);
    _vertShaderEntryPoint = std::move(entryPoint);
    return *this;
}

inline tpd::Material::Builder& tpd::Material::Builder::fragShader(const std::filesystem::path& path, std::string entryPoint) {
    _fragShaderCode = readShaderFile(path);
    _fragShaderEntryPoint = std::move(entryPoint);
    return *this;
}

inline tpd::Material::Builder& tpd::Material::Builder::withCustomLayout() {
    _customLayout = true;
    return *this;
}

inline tpd::Material::Builder& tpd::Material::Builder::minSampleShading(const float fraction) {
    _minSampleShading = fraction;
    return *this;
}

template<tpd::Shadable S>
std::unique_ptr<S> tpd::Material::Builder::build(const Renderer& renderer, std::unique_ptr<ShaderLayout> layout) const {
    auto pipelineInfo = renderer.getGraphicsPipelineInfo(_minSampleShading);
    const auto device = renderer.getVulkanDevice();

    // Shader stages
    const auto vertShaderModule = createShaderModule(device, _vertShaderCode);
    const auto fragShaderModule = createShaderModule(device, _fragShaderCode);
    const auto shaderStageInfos = std::array{
        vk::PipelineShaderStageCreateInfo{ {}, vk::ShaderStageFlagBits::eVertex,   vertShaderModule, _vertShaderEntryPoint.c_str() },
        vk::PipelineShaderStageCreateInfo{ {}, vk::ShaderStageFlagBits::eFragment, fragShaderModule, _fragShaderEntryPoint.c_str() },
    };
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStageInfos.size());
    pipelineInfo.pStages = shaderStageInfos.data();

    // Pipeline layout
    const auto withSharedLayout = layout ? !_customLayout : true;
    auto shaderLayout = layout ? std::move(layout) : getSharedLayoutBuilder().build(device);
    pipelineInfo.layout = shaderLayout->getPipelineLayout();

    // Create the graphics pipeline
    const auto pipeline = device.createGraphicsPipeline(nullptr, pipelineInfo).value;

    // We no longer need the shader modules once the pipeline is created
    device.destroyShaderModule(vertShaderModule);
    device.destroyShaderModule(fragShaderModule);

    return std::make_unique<S>(device, pipeline, std::move(shaderLayout), withSharedLayout);
}

inline tpd::Material::Material(
    const vk::Device device,
    const vk::Pipeline pipeline,
    std::unique_ptr<ShaderLayout> shaderLayout,
    const bool withSharedLayout) noexcept
    : _device{ device }
    , _pipeline{ pipeline }
    , _shaderLayout{ std::move(shaderLayout) }
    , _withSharedLayout { withSharedLayout } {
}

inline vk::Device tpd::Material::getVulkanDevice() const {
    return _device;
}

inline vk::Pipeline tpd::Material::getVulkanPipeline() const {
    return _pipeline;
}

inline vk::PipelineLayout tpd::Material::getVulkanPipelineLayout() const {
    return _shaderLayout->getPipelineLayout();
}

inline bool tpd::Material::builtWithSharedLayout() const {
    return _withSharedLayout;
}

inline tpd::ShaderLayout::Builder tpd::Material::getSharedLayoutBuilder(const uint32_t additionalSetCount) {
    using enum vk::ShaderStageFlagBits;
    return ShaderLayout::Builder()
        .descriptorSetCount(additionalSetCount + 1)
        .descriptor(0, 0, vk::DescriptorType::eUniformBuffer, 1, eVertex)              // drawable
        .descriptor(0, 1, vk::DescriptorType::eUniformBuffer, 1, eVertex | eFragment)  // camera
        .descriptor(0, 2, vk::DescriptorType::eUniformBuffer, 1, eFragment);           // light
}
