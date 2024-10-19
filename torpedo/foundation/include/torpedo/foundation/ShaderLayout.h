#pragma once

#include "torpedo/foundation/ShaderInstance.h"

namespace tpd {
    class ShaderLayout final {
    public:
        class Builder {
        public:
            Builder& descriptorSetCount(uint32_t count);

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
            std::vector<std::vector<vk::DescriptorSetLayoutBinding>> _descriptorSetLayoutBindingLists{};
            std::vector<std::vector<vk::DescriptorBindingFlags>> _descriptorSetBindingFlagLists{};

            std::vector<vk::PushConstantRange> _pushConstantRanges{};
        };

        ShaderLayout(
            vk::PipelineLayout pipelineLayout,
            std::vector<vk::DescriptorSetLayout>&& descriptorSetLayouts,
            std::vector<std::vector<vk::DescriptorSetLayoutBinding>>&& descriptorSetBindingFlagLists) noexcept;

        ShaderLayout(const ShaderLayout&) = delete;
        ShaderLayout& operator=(const ShaderLayout&) = delete;

        [[nodiscard]] std::unique_ptr<ShaderInstance> createInstance(
            vk::Device device,
            uint32_t instanceCount = 1,
            uint32_t firstSet = 0,
            vk::DescriptorPoolCreateFlags flags = {}) const;

        [[nodiscard]] vk::PipelineLayout getPipelineLayout() const;

        [[nodiscard]] bool empty() const;

        void dispose(vk::Device device) noexcept;

    private:
        vk::PipelineLayout _pipelineLayout;
        std::vector<vk::DescriptorSetLayout> _descriptorSetLayouts;
        std::vector<std::vector<vk::DescriptorSetLayoutBinding>> _descriptorSetLayoutBindingLists;
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::ShaderLayout::Builder& tpd::ShaderLayout::Builder::descriptorSetCount(const uint32_t count) {
    _descriptorSetLayoutBindingLists.resize(count);
    _descriptorSetBindingFlagLists.resize(count);
    return *this;
}

inline tpd::ShaderLayout::Builder& tpd::ShaderLayout::Builder::descriptor(
    const uint32_t set,
    const uint32_t binding,
    const vk::DescriptorType type,
    const uint32_t elementCount,
    const vk::ShaderStageFlags shaderStages,
    const vk::DescriptorBindingFlags flags)
{
    if (set >= _descriptorSetLayoutBindingLists.size()) {
        throw std::invalid_argument(
            "ShaderLayout::Builder - Descriptor set index out of range, did you forget to call Builder::descriptorSetCount?");
    }

    auto descriptorLayoutBinding = vk::DescriptorSetLayoutBinding{};
    descriptorLayoutBinding.binding = binding;
    descriptorLayoutBinding.descriptorType = type;
    descriptorLayoutBinding.descriptorCount = elementCount;
    descriptorLayoutBinding.stageFlags = shaderStages;

    _descriptorSetLayoutBindingLists[set].push_back(descriptorLayoutBinding);
    _descriptorSetBindingFlagLists[set].push_back(flags);

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
    std::vector<std::vector<vk::DescriptorSetLayoutBinding>>&& descriptorSetBindingFlagLists) noexcept
    : _pipelineLayout{ pipelineLayout }
    , _descriptorSetLayouts{ std::move(descriptorSetLayouts) }
    , _descriptorSetLayoutBindingLists{ std::move(descriptorSetBindingFlagLists) } {
}

inline vk::PipelineLayout tpd::ShaderLayout::getPipelineLayout() const {
    return _pipelineLayout;
}

inline bool tpd::ShaderLayout::empty() const {
    return _descriptorSetLayouts.empty();
}
