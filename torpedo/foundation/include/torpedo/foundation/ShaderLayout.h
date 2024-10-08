#pragma once

#include <vulkan/vulkan.hpp>

#include <filesystem>

namespace tpd {
    class ShaderInstance;

    class ShaderLayout final {
    public:
        class Builder {
        public:
            explicit Builder(uint32_t descriptorSetCount = 0, uint32_t descriptorBindingCount = 0);

            Builder& descriptor(
                uint32_t set,
                uint32_t binding,
                vk::DescriptorType type,
                uint32_t elementCount,
                vk::ShaderStageFlags shaderStages,
                vk::DescriptorBindingFlags flags = {});

            Builder& pushConstantRange(
                vk::ShaderStageFlags shaderStages,
                uint32_t byteOffset,
                uint32_t byteSize);

            [[nodiscard]] std::unique_ptr<ShaderLayout> build(vk::Device device);

        private:
            [[nodiscard]] std::vector<vk::DescriptorSetLayout> createDescriptorSetLayouts(vk::Device device) const;
            std::vector<std::vector<uint32_t>> _descriptorSetBindingLists{};
            std::vector<vk::DescriptorSetLayoutBinding> _descriptorSetLayoutBindings{};
            std::vector<vk::DescriptorBindingFlags> _descriptorBindingFlags{};

            std::vector<vk::PushConstantRange> _pushConstantRanges{};
        };

        ShaderLayout(
            vk::PipelineLayout pipelineLayout,
            std::vector<vk::DescriptorSetLayout>&& descriptorSetLayouts,
            std::vector<vk::DescriptorSetLayoutBinding>&& descriptorSetLayoutBindings) noexcept;

        ShaderLayout(const ShaderLayout&) = delete;
        ShaderLayout& operator=(const ShaderLayout&) = delete;

        [[nodiscard]] vk::PipelineLayout getPipelineLayout() const;

        void dispose(vk::Device device) noexcept;

        [[nodiscard]] std::unique_ptr<ShaderInstance> createInstance(
            vk::Device device,
            uint32_t instanceCount = 1,
            vk::DescriptorPoolCreateFlags flags = {}) const;

    private:
        vk::PipelineLayout _pipelineLayout;
        std::vector<vk::DescriptorSetLayout> _descriptorSetLayouts;
        std::vector<vk::DescriptorSetLayoutBinding> _descriptorSetLayoutBindings;
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::ShaderLayout::Builder::Builder(const uint32_t descriptorSetCount, const uint32_t descriptorBindingCount) {
    _descriptorSetBindingLists.resize(descriptorSetCount);
    _descriptorSetLayoutBindings.resize(descriptorBindingCount);
    _descriptorBindingFlags.resize(descriptorBindingCount);
}

inline tpd::ShaderLayout::Builder& tpd::ShaderLayout::Builder::descriptor(
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

inline tpd::ShaderLayout::Builder&tpd::ShaderLayout::Builder::pushConstantRange(
    const vk::ShaderStageFlags shaderStages,
    const uint32_t byteOffset,
    const uint32_t byteSize)
{
    _pushConstantRanges.emplace_back(shaderStages, byteOffset, byteSize);
    return *this;
}

inline tpd::ShaderLayout::ShaderLayout(
    const vk::PipelineLayout pipelineLayout,
    std::vector<vk::DescriptorSetLayout>&& descriptorSetLayouts,
    std::vector<vk::DescriptorSetLayoutBinding>&& descriptorSetLayoutBindings) noexcept
    : _pipelineLayout{ pipelineLayout }
    , _descriptorSetLayouts{ std::move(descriptorSetLayouts) }
    , _descriptorSetLayoutBindings{ std::move(descriptorSetLayoutBindings) } {
}

inline vk::PipelineLayout tpd::ShaderLayout::getPipelineLayout() const {
    return _pipelineLayout;
}

inline void tpd::ShaderLayout::dispose(const vk::Device device) noexcept {
    device.destroyPipelineLayout(_pipelineLayout);
    std::ranges::for_each(_descriptorSetLayouts, [&device](const auto& it) { device.destroyDescriptorSetLayout(it); });
    _descriptorSetLayouts.clear();
    _descriptorSetLayoutBindings.clear();
}
