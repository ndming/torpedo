#include "torpedo/foundation/ShaderLayout.h"
#include "torpedo/foundation/ShaderInstance.h"

#include <ranges>
#include <unordered_map>

std::unique_ptr<tpd::ShaderLayout> tpd::ShaderLayout::Builder::build(const vk::Device device) {
    // Descriptor layouts, one for each descriptor set
    auto descriptorSetLayouts = std::vector<vk::DescriptorSetLayout>(_descriptorSetLayoutBindingLists.size());
    for (uint32_t i = 0; i < _descriptorSetLayoutBindingLists.size(); i++) {
        auto bindingFlagInfo = vk::DescriptorSetLayoutBindingFlagsCreateInfo{};
        bindingFlagInfo.bindingCount  = static_cast<uint32_t>(_descriptorSetBindingFlagLists[i].size());
        bindingFlagInfo.pBindingFlags = _descriptorSetBindingFlagLists[i].data();

        auto layoutInfo = vk::DescriptorSetLayoutCreateInfo{};
        layoutInfo.bindingCount = static_cast<uint32_t>(_descriptorSetLayoutBindingLists[i].size());
        layoutInfo.pBindings = _descriptorSetLayoutBindingLists[i].data();
        layoutInfo.pNext = &bindingFlagInfo;

        descriptorSetLayouts[i] = device.createDescriptorSetLayout(layoutInfo);
    }

    // Pipeline layout
    auto pipelineLayoutInfo = vk::PipelineLayoutCreateInfo{};
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(_pushConstantRanges.size());
    pipelineLayoutInfo.pPushConstantRanges = _pushConstantRanges.data();
    const auto pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);

    return std::make_unique<ShaderLayout>(pipelineLayout, std::move(descriptorSetLayouts), std::move(_descriptorSetLayoutBindingLists));
}

std::unique_ptr<tpd::ShaderInstance> tpd::ShaderLayout::createInstance(
    const vk::Device device,
    const uint32_t instanceCount,
    const uint32_t firstSet,
    const vk::DescriptorPoolCreateFlags flags) const
{
    // Create an empty ShaderInstance if the number of sets to include is zero
    if (firstSet >= _descriptorSetLayouts.size()) {
        return std::make_unique<ShaderInstance>();
    }

    // Count how many descriptors of each type in this pipeline, ...
    auto descriptorTypeCounts = std::unordered_map<vk::DescriptorType, uint32_t>{};
    for (const auto binding : _descriptorSetLayoutBindingLists | std::views::drop(firstSet) | std::views::join) {
        descriptorTypeCounts[binding.descriptorType] += binding.descriptorCount;
    }
    // ... then compute the pool sizes for each descriptor type
    auto poolSizes = std::vector<vk::DescriptorPoolSize>{};
    for (const auto& [descriptorType, descriptorCount] : descriptorTypeCounts) {
        poolSizes.emplace_back(descriptorType, instanceCount * descriptorCount);
    }

    // Create a descriptor pool
    auto poolInfo = vk::DescriptorPoolCreateInfo{};
    poolInfo.flags = flags;
    poolInfo.maxSets = instanceCount * (_descriptorSetLayoutBindingLists.size() - firstSet);
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    const auto descriptorPool = device.createDescriptorPool(poolInfo);

    // Allocate a descriptor set for each instance
    const auto layouts =  std::views::repeat(_descriptorSetLayouts | std::views::drop(firstSet), instanceCount)
        | std::views::join
        | std::ranges::to<std::vector>();
    auto allocInfo = vk::DescriptorSetAllocateInfo{};
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    allocInfo.pSetLayouts = layouts.data();

    const auto perInstanceSetCount = _descriptorSetLayouts.size() - firstSet;
    return std::make_unique<ShaderInstance>(perInstanceSetCount, descriptorPool, device.allocateDescriptorSets(allocInfo));
}

void tpd::ShaderLayout::dispose(const vk::Device device) noexcept {
    device.destroyPipelineLayout(_pipelineLayout);
    std::ranges::for_each(_descriptorSetLayouts, [&device](const auto& it) { device.destroyDescriptorSetLayout(it); });
    _descriptorSetLayouts.clear();
    _descriptorSetLayoutBindingLists.clear();
}
