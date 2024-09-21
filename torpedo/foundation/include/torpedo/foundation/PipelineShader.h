#pragma once

#include <vulkan/vulkan.hpp>

#include <filesystem>
#include <memory>

namespace tpd {
    class PipelineInstance;

    class PipelineShader final {
    public:
        class Builder {
        public:
            explicit Builder(uint32_t descriptorSetCount = 0, uint32_t descriptorBindingCount = 0);

            Builder& shader(
                const std::filesystem::path& path,
                vk::ShaderStageFlagBits stage,
                std::string entryPoint = "main");

            Builder& descriptor(
                uint32_t set,
                uint32_t binding,
                vk::DescriptorType type,
                uint32_t elementCount,
                vk::ShaderStageFlags shaderStages,
                vk::DescriptorBindingFlags flags = {});

            Builder& pushConstantRange(vk::ShaderStageFlags shaderStages, uint32_t byteOffset, uint32_t byteSize);

            [[nodiscard]] std::unique_ptr<PipelineShader> build(
                vk::GraphicsPipelineCreateInfo* pipelineInfo,
                vk::PhysicalDevice physicalDevice, vk::Device device);

        private:
            [[nodiscard]] static std::vector<std::byte> readShaderFile(const std::filesystem::path& path);
            [[nodiscard]] std::vector<vk::ShaderModule> createShaderModules(vk::Device device) const;
            std::vector<std::vector<std::byte>> _shaderCodes{};
            std::vector<std::string> _shaderEntryPoints{};
            std::vector<vk::ShaderStageFlagBits> _shaderStages{};

            [[nodiscard]] std::vector<vk::DescriptorSetLayout> createDescriptorSetLayouts(vk::Device device) const;
            std::vector<std::vector<uint32_t>> _descriptorSetBindingLists{};
            std::vector<vk::DescriptorSetLayoutBinding> _descriptorSetLayoutBindings{};
            std::vector<vk::DescriptorBindingFlags> _descriptorBindingFlags{};

            void checkPushConstants(vk::PhysicalDevice physicalDevice) const;
            std::vector<vk::PushConstantRange> _pushConstantRanges{};
        };

        PipelineShader(
            vk::PipelineLayout pipelineLayout, vk::Pipeline pipeline,
            const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts,
            const std::vector<vk::DescriptorSetLayoutBinding>& descriptorBindings);

        PipelineShader(
            vk::PipelineLayout pipelineLayout, vk::Pipeline pipeline,
            std::vector<vk::DescriptorSetLayout>&& descriptorSetLayouts,
            std::vector<vk::DescriptorSetLayoutBinding>&& descriptorBindings) noexcept;

        PipelineShader(const PipelineShader&) = delete;
        PipelineShader& operator=(const PipelineShader&) = delete;

        [[nodiscard]] std::unique_ptr<PipelineInstance> createInstance(
            vk::Device device,
            uint32_t instanceCount = 1,
            vk::DescriptorPoolCreateFlags flags = {}) const;

        [[nodiscard]] vk::Pipeline getPipeline() const;
        [[nodiscard]] vk::PipelineLayout getPipelineLayout() const;

        void destroy(vk::Device device) noexcept;

    private:
        vk::PipelineLayout _pipelineLayout;
        vk::Pipeline _pipeline;

        std::vector<vk::DescriptorSetLayout> _descriptorSetLayouts;
        std::vector<vk::DescriptorSetLayoutBinding> _descriptorSetLayoutBindings;
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::PipelineShader::Builder::Builder(const uint32_t descriptorSetCount, const uint32_t descriptorBindingCount) {
    _descriptorSetBindingLists.resize(descriptorSetCount);
    _descriptorSetLayoutBindings.resize(descriptorBindingCount);
    _descriptorBindingFlags.resize(descriptorBindingCount);
}

inline tpd::PipelineShader::Builder& tpd::PipelineShader::Builder::shader(
    const std::filesystem::path& path,
    const vk::ShaderStageFlagBits stage,
    std::string entryPoint)
{
    _shaderCodes.push_back(readShaderFile(path));
    _shaderEntryPoints.push_back(std::move(entryPoint));
    _shaderStages.push_back(stage);
    return *this;
}

inline tpd::PipelineShader::Builder& tpd::PipelineShader::Builder::descriptor(
    const uint32_t set,
    const uint32_t binding,
    const vk::DescriptorType type,
    const uint32_t elementCount,
    const vk::ShaderStageFlags shaderStages,
    const vk::DescriptorBindingFlags flags)
{
    _descriptorSetBindingLists[set].push_back(binding);
    _descriptorSetLayoutBindings[binding] = { binding, type, elementCount, shaderStages };
    _descriptorBindingFlags[binding] = flags;
    return *this;
}

inline tpd::PipelineShader::Builder&tpd::PipelineShader::Builder::pushConstantRange(
    const vk::ShaderStageFlags shaderStages,
    const uint32_t byteOffset,
    const uint32_t byteSize)
{
    _pushConstantRanges.emplace_back(shaderStages, byteOffset, byteSize);
    return *this;
}

inline tpd::PipelineShader::PipelineShader(
    const vk::PipelineLayout pipelineLayout,
    const vk::Pipeline pipeline,
    const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts,
    const std::vector<vk::DescriptorSetLayoutBinding>& descriptorBindings)
: _pipelineLayout{ pipelineLayout }, _pipeline{ pipeline }, _descriptorSetLayouts{ descriptorSetLayouts },
_descriptorSetLayoutBindings{ descriptorBindings } {
}

inline tpd::PipelineShader::PipelineShader(
    const vk::PipelineLayout pipelineLayout,
    const vk::Pipeline pipeline,
    std::vector<vk::DescriptorSetLayout>&& descriptorSetLayouts,
    std::vector<vk::DescriptorSetLayoutBinding>&& descriptorBindings) noexcept
: _pipelineLayout{ pipelineLayout }, _pipeline{ pipeline }, _descriptorSetLayouts{ std::move(descriptorSetLayouts) },
_descriptorSetLayoutBindings{ std::move(descriptorBindings) } {
}

inline vk::Pipeline tpd::PipelineShader::getPipeline() const {
    return _pipeline;
}

inline vk::PipelineLayout tpd::PipelineShader::getPipelineLayout() const {
    return _pipelineLayout;
}
