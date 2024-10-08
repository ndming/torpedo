#include "torpedo/foundation/ShaderLayout.h"
#include "torpedo/foundation/ShaderInstance.h"

#include <fstream>
#include <ranges>
#include <unordered_map>

std::unique_ptr<tpd::ShaderLayout> tpd::ShaderLayout::Builder::build(const vk::Device device) {
    // Descriptor layouts, one for each descriptor set
    auto descriptorSetLayouts = createDescriptorSetLayouts(device);

    // Pipeline layout
    auto pipelineLayoutInfo = vk::PipelineLayoutCreateInfo{};
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(_pushConstantRanges.size());
    pipelineLayoutInfo.pPushConstantRanges = _pushConstantRanges.data();
    const auto pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);

    return std::make_unique<ShaderLayout>(pipelineLayout, std::move(descriptorSetLayouts), std::move(_descriptorSetLayoutBindings));
}

std::vector<vk::DescriptorSetLayout> tpd::ShaderLayout::Builder::createDescriptorSetLayouts(const vk::Device device) const {
    auto descriptorSetLayouts = std::vector<vk::DescriptorSetLayout>{};
    for (const auto& bindings : _descriptorSetBindingLists) {
        auto descriptorSetBindings = std::vector<vk::DescriptorSetLayoutBinding>{};
        auto descriptorSetBindingFlags = std::vector<vk::DescriptorBindingFlags>{};
        for (const auto binding : bindings) {
            descriptorSetBindings.push_back(_descriptorSetLayoutBindings[binding]);
            descriptorSetBindingFlags.push_back(_descriptorBindingFlags[binding]);
        }

        auto bindingFlagInfo = vk::DescriptorSetLayoutBindingFlagsCreateInfo{};
        bindingFlagInfo.bindingCount = static_cast<uint32_t>(descriptorSetBindingFlags.size());
        bindingFlagInfo.pBindingFlags = descriptorSetBindingFlags.data();

        auto layoutInfo = vk::DescriptorSetLayoutCreateInfo{};
        layoutInfo.bindingCount = static_cast<uint32_t>(descriptorSetBindings.size());
        layoutInfo.pBindings = descriptorSetBindings.data();
        layoutInfo.pNext = &bindingFlagInfo;

        descriptorSetLayouts.push_back(device.createDescriptorSetLayout(layoutInfo));
    }
    return descriptorSetLayouts;
}

std::unique_ptr<tpd::ShaderInstance> tpd::ShaderLayout::createInstance(
    const vk::Device device,
    const uint32_t instanceCount,
    const vk::DescriptorPoolCreateFlags flags) const
{
    // Count how many descriptors of each type in this pipeline
    auto descriptorTypeNumbers = std::unordered_map<vk::DescriptorType, uint32_t>{};
    for (const auto binding : _descriptorSetLayoutBindings) {
        descriptorTypeNumbers[binding.descriptorType] += binding.descriptorCount;
    }

    // Compute the pool size
    auto poolSizes = std::vector<vk::DescriptorPoolSize>{};
    for (const auto& [descriptorType, descriptorCount] : descriptorTypeNumbers) {
        poolSizes.emplace_back(descriptorType, instanceCount * descriptorCount);
    }

    // Create a descriptor pool
    auto poolInfo = vk::DescriptorPoolCreateInfo{};
    poolInfo.flags = flags;
    poolInfo.maxSets = instanceCount;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    const auto descriptorPool = device.createDescriptorPool(poolInfo);

    // Allocate a descriptor set for each instance
    const auto layouts =  std::views::repeat(_descriptorSetLayouts, instanceCount) | std::views::join | std::ranges::to<std::vector>();
    auto allocInfo = vk::DescriptorSetAllocateInfo{};
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    allocInfo.pSetLayouts = layouts.data();

    const auto perInstanceSetCount = static_cast<uint32_t>(_descriptorSetLayouts.size());
    return std::make_unique<ShaderInstance>(perInstanceSetCount, descriptorPool, device.allocateDescriptorSets(allocInfo));
}
