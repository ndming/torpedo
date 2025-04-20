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

            [[nodiscard]] ShaderLayout build(vk::Device device, vk::PipelineLayout* pipelineLayout);

        private:
            [[nodiscard]] std::vector<vk::DescriptorSetLayout> createLayouts(vk::Device device) const;
            [[nodiscard]] vk::PipelineLayout createPipelineLayout(vk::Device device, const std::vector<vk::DescriptorSetLayout>& layouts) const;

            std::vector<std::vector<vk::DescriptorSetLayoutBinding>> _descriptorSetLayoutBindingLists{};

            std::vector<std::vector<vk::DescriptorBindingFlags>> _descriptorSetBindingFlagLists{};
            std::vector<vk::PushConstantRange> _pushConstantRanges{};
        };

        ShaderLayout() noexcept = default;
        ShaderLayout(
            std::vector<vk::DescriptorSetLayout>&& descriptorSetLayouts,
            std::vector<std::vector<vk::DescriptorSetLayoutBinding>>&& descriptorSetBindingFlagLists) noexcept;

        ShaderLayout(ShaderLayout&&) noexcept = default;
        ShaderLayout& operator=(ShaderLayout&&) noexcept = default;

        [[nodiscard]] ShaderInstance createInstance(
            vk::Device device,
            uint32_t instanceCount = 1,
            uint32_t firstSetIndex = 0,
            vk::DescriptorPoolCreateFlags flags = {}) const;

        [[nodiscard]] bool empty() const noexcept;
        void destroy(vk::Device device) noexcept;

    private:
        [[nodiscard]] vk::DescriptorPool createDescriptorPool(
            vk::Device device,
            uint32_t instanceCount,
            uint32_t firstSetIndex,
            vk::DescriptorPoolCreateFlags flags) const;

        [[nodiscard]] std::vector<vk::DescriptorSet> createDescriptorSets(
            vk::Device device,
            vk::DescriptorPool descriptorPool,
            uint32_t firstSetIndex,
            uint32_t instanceCount) const;

        /*------------------*/

        std::vector<vk::DescriptorSetLayout> _setLayouts{};
        std::vector<std::vector<vk::DescriptorSetLayoutBinding>> _bindingLists{};

        /*------------------*/
    };
} // namespace tpd

inline tpd::ShaderLayout::Builder& tpd::ShaderLayout::Builder::descriptorSetCount(const uint32_t count) {
    if (count > ShaderInstance::MAX_DESCRIPTOR_SETS) [[unlikely]] {
        throw std::invalid_argument(
            "ShaderLayout::Builder - Only allow up to " + std::to_string(ShaderInstance::MAX_DESCRIPTOR_SETS) + " descriptor sets!");
    }
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
            "ShaderLayout::Builder - Descriptor set index out of range: "
            "did you forget to call Builder::descriptorSetCount()?");
    }

    _descriptorSetLayoutBindingLists[set].emplace_back(binding, type, elementCount, shaderStages);
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
    std::vector<vk::DescriptorSetLayout>&& descriptorSetLayouts,
    std::vector<std::vector<vk::DescriptorSetLayoutBinding>>&& descriptorSetBindingFlagLists) noexcept
    : _setLayouts{ std::move(descriptorSetLayouts) }
    , _bindingLists{ std::move(descriptorSetBindingFlagLists) }
{}

inline bool tpd::ShaderLayout::empty() const noexcept {
    return _setLayouts.empty();
}
