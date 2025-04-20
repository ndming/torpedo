#include "torpedo/foundation/ShaderLayout.h"

#include <ranges>
#include <unordered_map>

tpd::ShaderLayout tpd::ShaderLayout::Builder::build(const vk::Device device, vk::PipelineLayout* pipelineLayout) {
    auto layouts = createLayouts(device);
    *pipelineLayout = createPipelineLayout(device, layouts);
    return ShaderLayout{ std::move(layouts), std::move(_descriptorSetLayoutBindingLists) };
}

std::vector<vk::DescriptorSetLayout> tpd::ShaderLayout::Builder::createLayouts(const vk::Device device) const {
    // Create one descriptor set layout for each descriptor set
    auto descriptorSetLayouts = std::vector<vk::DescriptorSetLayout>{};
    descriptorSetLayouts.resize(_descriptorSetLayoutBindingLists.size());

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
    return descriptorSetLayouts;
}

vk::PipelineLayout tpd::ShaderLayout::Builder::createPipelineLayout(
    const vk::Device device,
    const std::vector<vk::DescriptorSetLayout>& layouts) const
{
    auto pipelineLayoutInfo = vk::PipelineLayoutCreateInfo{};
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
    pipelineLayoutInfo.pSetLayouts = layouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(_pushConstantRanges.size());
    pipelineLayoutInfo.pPushConstantRanges = _pushConstantRanges.data();
    return device.createPipelineLayout(pipelineLayoutInfo);
}

tpd::ShaderInstance tpd::ShaderLayout::createInstance(
    const vk::Device device,
    const uint32_t instanceCount,
    const uint32_t firstSetIndex,
    const vk::DescriptorPoolCreateFlags flags) const
{
    if (instanceCount > ShaderInstance::MAX_INSTANCES) {
        throw std::runtime_error("ShaderLayout - Only allow up to " + std::to_string(ShaderInstance::MAX_INSTANCES) + " instances");
    }

    // Create an empty ShaderInstance if the number of sets to include is zero
    // This also means the shader layout contains no descriptor sets
    if (firstSetIndex >= _setLayouts.size()) [[unlikely]] {
        return ShaderInstance{};
    }

    const auto descriptorPool = createDescriptorPool(device, instanceCount, firstSetIndex, flags);
    const auto descriptorSets = createDescriptorSets(device, descriptorPool, firstSetIndex, instanceCount);

    const auto setCountPerInstance = static_cast<uint32_t>(_setLayouts.size() - firstSetIndex);
    return ShaderInstance{ descriptorPool, setCountPerInstance, descriptorSets };
}

vk::DescriptorPool tpd::ShaderLayout::createDescriptorPool(
    const vk::Device device,
    const uint32_t instanceCount,
    const uint32_t firstSetIndex,
    const vk::DescriptorPoolCreateFlags flags) const 
{
    // Count how many descriptors of each type in this pipeline, ...
    auto descriptorTypeCounts = std::unordered_map<vk::DescriptorType, uint32_t>{};
    for (const auto binding : _bindingLists | std::views::drop(firstSetIndex) | std::views::join) {
        descriptorTypeCounts[binding.descriptorType] += binding.descriptorCount;
    }
    // ... then compute the pool sizes for each descriptor type
    auto poolSizes = std::vector<vk::DescriptorPoolSize>{};
    poolSizes.reserve(descriptorTypeCounts.size());
    for (const auto& [descriptorType, descriptorCount] : descriptorTypeCounts) {
        poolSizes.emplace_back(descriptorType, instanceCount * descriptorCount);
    }
    
    const auto maxSets = static_cast<uint32_t>(instanceCount * (_bindingLists.size() - firstSetIndex));
    return device.createDescriptorPool({ flags, maxSets, poolSizes });
}

std::vector<vk::DescriptorSet> tpd::ShaderLayout::createDescriptorSets(
    const vk::Device device,
    const vk::DescriptorPool descriptorPool,
    const uint32_t firstSetIndex,
    const uint32_t instanceCount) const
{
    // Allocate one descriptor set for each instance
    const auto layouts =  std::views::repeat(_setLayouts | std::views::drop(firstSetIndex), instanceCount)
        | std::views::join
        | std::ranges::to<std::vector>();

    const auto allocInfo = vk::DescriptorSetAllocateInfo{}
        .setDescriptorPool(descriptorPool)
        .setSetLayouts(layouts);
    return device.allocateDescriptorSets(allocInfo);
}

void tpd::ShaderLayout::destroy(const vk::Device device) noexcept {
    std::ranges::for_each(_setLayouts, [&device](const auto& it) { device.destroyDescriptorSetLayout(it); });
    _setLayouts.clear();
    _bindingLists.clear();
}
