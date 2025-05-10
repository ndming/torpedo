#pragma once

#include "torpedo/foundation/ShaderInstance.h"

#include <ranges>
#include <unordered_map>

namespace tpd {
    template<uint32_t SetCount = 0>
    class ShaderLayout final {
    public:
        class Builder {
        public:
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
            [[nodiscard]] std::array<vk::DescriptorSetLayout, SetCount> createLayouts(vk::Device device) const;
            [[nodiscard]] vk::PipelineLayout createPipelineLayout(
                vk::Device device, const std::array<vk::DescriptorSetLayout, SetCount>& layouts) const;

            std::array<std::vector<vk::DescriptorSetLayoutBinding>, SetCount> _descriptorSetLayoutBindingLists{};

            std::array<std::vector<vk::DescriptorBindingFlags>, SetCount> _descriptorSetBindingFlagLists{};
            std::vector<vk::PushConstantRange> _pushConstantRanges{};
        };

        ShaderLayout() noexcept = default;

        ShaderLayout(
            std::array<vk::DescriptorSetLayout, SetCount> descriptorSetLayouts,
            std::array<std::vector<vk::DescriptorSetLayoutBinding>, SetCount> descriptorSetBindingLists);

        ShaderLayout(ShaderLayout&&) noexcept = default;
        ShaderLayout& operator=(ShaderLayout&&) noexcept = default;

        [[nodiscard]] ShaderInstance<SetCount> createInstance(vk::Device device, vk::DescriptorPoolCreateFlags flags = {}) const;

        void destroy(vk::Device device) noexcept;

    private:
        [[nodiscard]] vk::DescriptorPool createDescriptorPool(vk::Device device, vk::DescriptorPoolCreateFlags flags) const;

        /*------------------*/

        std::array<vk::DescriptorSetLayout, SetCount> _setLayouts{};
        std::array<std::vector<vk::DescriptorSetLayoutBinding>, SetCount> _bindingLists{};

        /*------------------*/
    };
} // namespace tpd

template<uint32_t SetCount>
typename tpd::ShaderLayout<SetCount>::Builder& tpd::ShaderLayout<SetCount>::Builder::descriptor(
    const uint32_t set, const uint32_t binding, const vk::DescriptorType type, const uint32_t elementCount,
    const vk::ShaderStageFlags shaderStages, const vk::DescriptorBindingFlags flags)
{
    _descriptorSetLayoutBindingLists[set].emplace_back(binding, type, elementCount, shaderStages);
    _descriptorSetBindingFlagLists[set].push_back(flags);
    return *this;
}

template<uint32_t SetCount>
typename tpd::ShaderLayout<SetCount>::Builder& tpd::ShaderLayout<SetCount>::Builder::pushConstantRange(
    const vk::ShaderStageFlags shaderStages, const uint32_t byteOffset, const uint32_t byteSize)
{
    _pushConstantRanges.emplace_back(shaderStages, byteOffset, byteSize);
    return *this;
}

template<uint32_t SetCount>
tpd::ShaderLayout<SetCount> tpd::ShaderLayout<SetCount>::Builder::build(
    const vk::Device device,
    vk::PipelineLayout* pipelineLayout)
{
    const auto layouts = createLayouts(device);
    *pipelineLayout = createPipelineLayout(device, layouts);
    return ShaderLayout{ layouts, std::move(_descriptorSetLayoutBindingLists) };
}

template<uint32_t SetCount>
std::array<vk::DescriptorSetLayout, SetCount> tpd::ShaderLayout<SetCount>::Builder::createLayouts(const vk::Device device) const {
    // Create one descriptor set layout for each descriptor set
    auto descriptorSetLayouts = std::array<vk::DescriptorSetLayout, SetCount>{};

    for (uint32_t i = 0; i < SetCount; i++) {
        auto bindingFlagInfo = vk::DescriptorSetLayoutBindingFlagsCreateInfo{};
        bindingFlagInfo.bindingCount  = static_cast<uint32_t>(_descriptorSetBindingFlagLists[i].size());
        bindingFlagInfo.pBindingFlags = _descriptorSetBindingFlagLists[i].data();

        auto layoutInfo = vk::DescriptorSetLayoutCreateInfo{};
        layoutInfo.bindingCount = static_cast<uint32_t>(_descriptorSetLayoutBindingLists[i].size());
        layoutInfo.pBindings = _descriptorSetLayoutBindingLists[i].data();
        layoutInfo.pNext = &bindingFlagInfo;

        descriptorSetLayouts[i] = device.createDescriptorSetLayout(layoutInfo);
    }
    return descriptorSetLayouts;
}

template<uint32_t SetCount>
vk::PipelineLayout tpd::ShaderLayout<SetCount>::Builder::createPipelineLayout(
    const vk::Device device,
    const std::array<vk::DescriptorSetLayout, SetCount>& layouts) const
{
    auto pipelineLayoutInfo = vk::PipelineLayoutCreateInfo{};
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
    pipelineLayoutInfo.pSetLayouts = layouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(_pushConstantRanges.size());
    pipelineLayoutInfo.pPushConstantRanges = _pushConstantRanges.data();
    return device.createPipelineLayout(pipelineLayoutInfo);
}

template<uint32_t SetCount>
tpd::ShaderLayout<SetCount>::ShaderLayout(
    const std::array<vk::DescriptorSetLayout, SetCount> descriptorSetLayouts,
    const std::array<std::vector<vk::DescriptorSetLayoutBinding>, SetCount> descriptorSetBindingLists)
    : _setLayouts{ descriptorSetLayouts }
    , _bindingLists{ std::move(descriptorSetBindingLists) }
{}

template<uint32_t SetCount>
tpd::ShaderInstance<SetCount> tpd::ShaderLayout<SetCount>::createInstance(
    const vk::Device device,
    const vk::DescriptorPoolCreateFlags flags) const
{
    if (SetCount == 0) [[unlikely]] {
        return ShaderInstance<SetCount>{};
    }

    const auto descriptorPool = createDescriptorPool(device, flags);

    const auto allocInfo = vk::DescriptorSetAllocateInfo{}
        .setDescriptorPool(descriptorPool)
        .setSetLayouts(_setLayouts);
    return ShaderInstance<SetCount>{ descriptorPool, device.allocateDescriptorSets(allocInfo) };
}

template<uint32_t SetCount>
vk::DescriptorPool tpd::ShaderLayout<SetCount>::createDescriptorPool(
    const vk::Device device,
    const vk::DescriptorPoolCreateFlags flags) const
{
    // Count how many descriptors of each type in this pipeline, ...
    auto descriptorTypeCounts = std::unordered_map<vk::DescriptorType, uint32_t>{};
    for (const auto binding : _bindingLists | std::views::join) {
        descriptorTypeCounts[binding.descriptorType] += binding.descriptorCount;
    }
    // ... then compute the pool sizes for each descriptor type
    auto poolSizes = std::vector<vk::DescriptorPoolSize>{};
    poolSizes.reserve(descriptorTypeCounts.size());
    for (const auto& [descriptorType, descriptorCount] : descriptorTypeCounts) {
        poolSizes.emplace_back(descriptorType, descriptorCount);
    }

    return device.createDescriptorPool({ flags, SetCount, poolSizes });
}

template<uint32_t SetCount>
void tpd::ShaderLayout<SetCount>::destroy(const vk::Device device) noexcept {
    std::ranges::for_each(_setLayouts, [&device](const auto& it) { device.destroyDescriptorSetLayout(it); });
}
