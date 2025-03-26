#include "torpedo/foundation/ShaderLayout.h"

#include <ranges>
#include <unordered_map>

tpd::ShaderLayout tpd::ShaderLayout::Builder::build(const vk::Device device) {
    auto layouts = createLayouts(device);
    const auto pipelineLayout = createPipelineLayout(device, layouts);
    return ShaderLayout{ pipelineLayout, std::move(layouts), std::move(_descriptorSetLayoutBindingLists) };
}

std::unique_ptr<tpd::ShaderLayout, tpd::Deleter<tpd::ShaderLayout>> tpd::ShaderLayout::Builder::buildUnique(const vk::Device device) {
    auto layouts = createLayouts(device);
    const auto pipelineLayout = createPipelineLayout(device, layouts);
    return foundation::make_unique<ShaderLayout>(_pool, pipelineLayout, std::move(layouts), std::move(_descriptorSetLayoutBindingLists));
}

std::pmr::vector<vk::DescriptorSetLayout> tpd::ShaderLayout::Builder::createLayouts(const vk::Device device) {
    // Create one descriptor set layout for each descriptor set
    auto descriptorSetLayouts = std::pmr::vector<vk::DescriptorSetLayout>{ _pool };
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
    const std::pmr::vector<vk::DescriptorSetLayout>& layouts) 
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
    // Create an empty ShaderInstance if the number of sets to include is zero
    // This also means the shader layout contains no descriptor sets
    if (firstSetIndex >= _descriptorSetLayouts.size()) {
        return ShaderInstance{};
    }

    const auto descriptorPool = createDescriptorPool(device, instanceCount, firstSetIndex, flags);
    auto descriptorSets = createDescriptorSets(device, descriptorPool, firstSetIndex, instanceCount);

    const auto setCountPerInstance = static_cast<uint32_t>(_descriptorSetLayouts.size() - firstSetIndex);
    return ShaderInstance{ descriptorPool, setCountPerInstance, std::move(descriptorSets) };
}

std::unique_ptr<tpd::ShaderInstance, tpd::Deleter<tpd::ShaderInstance>> tpd::ShaderLayout::createInstance(
    std::pmr::memory_resource* pool,
    const vk::Device device,
    const uint32_t instanceCount,
    const uint32_t firstSetIndex,
    const vk::DescriptorPoolCreateFlags flags) const
{
    // Create an empty ShaderInstance if the number of sets to include is zero
    // This also means the shader layout contains no descriptor sets
    if (firstSetIndex >= _descriptorSetLayouts.size()) {
        return foundation::make_unique<ShaderInstance>(pool);
    }

    const auto descriptorPool = createDescriptorPool(device, instanceCount, firstSetIndex, flags);
    auto descriptorSets = createDescriptorSets(device, descriptorPool, firstSetIndex, instanceCount, pool);

    const auto setCountPerInstance = static_cast<uint32_t>(_descriptorSetLayouts.size() - firstSetIndex);
    return foundation::make_unique<ShaderInstance>(pool, descriptorPool, setCountPerInstance, std::move(descriptorSets));
}

vk::DescriptorPool tpd::ShaderLayout::createDescriptorPool(
    const vk::Device device,
    const uint32_t instanceCount,
    const uint32_t firstSetIndex,
    const vk::DescriptorPoolCreateFlags flags) const 
{
    // Count how many descriptors of each type in this pipeline, ...
    auto descriptorTypeCounts = std::unordered_map<vk::DescriptorType, uint32_t>{};
    for (const auto binding : _descriptorSetLayoutBindingLists | std::views::drop(firstSetIndex) | std::views::join) {
        descriptorTypeCounts[binding.descriptorType] += binding.descriptorCount;
    }
    // ... then compute the pool sizes for each descriptor type
    auto poolSizes = std::vector<vk::DescriptorPoolSize>{};
    poolSizes.reserve(descriptorTypeCounts.size());
    for (const auto& [descriptorType, descriptorCount] : descriptorTypeCounts) {
        poolSizes.emplace_back(descriptorType, instanceCount * descriptorCount);
    }
    
    const auto maxSets = static_cast<uint32_t>(instanceCount * (_descriptorSetLayoutBindingLists.size() - firstSetIndex));
    return device.createDescriptorPool({ flags, maxSets, poolSizes });
}

std::pmr::vector<VkDescriptorSet> tpd::ShaderLayout::createDescriptorSets(
    const vk::Device device,
    const vk::DescriptorPool descriptorPool,
    const uint32_t firstSetIndex,
    const uint32_t instanceCount,
    std::pmr::memory_resource* pool) const
{
    // Allocate one descriptor set for each instance
    const auto layouts =  std::views::repeat(_descriptorSetLayouts | std::views::drop(firstSetIndex), instanceCount)
        | std::views::join
        | std::views::transform([](const auto& it) { return static_cast<VkDescriptorSetLayout>(it); })
        | std::ranges::to<std::vector>();
    const auto allocInfo = VkDescriptorSetAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data()
    };

    // Use the C-API to allocate descriptor sets into a vector with a custom memory resource
    auto descriptorSets = std::pmr::vector<VkDescriptorSet>{ pool };
    descriptorSets.resize(layouts.size());
    vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data());

    return descriptorSets;
}

void tpd::ShaderLayout::destroy(const vk::Device device) noexcept {
    device.destroyPipelineLayout(_pipelineLayout);
    std::ranges::for_each(_descriptorSetLayouts, [&device](const auto& it) { device.destroyDescriptorSetLayout(it); });
    _descriptorSetLayouts.clear();
    _descriptorSetLayoutBindingLists.clear();
}
