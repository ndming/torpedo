#include "torpedo/foundation/PipelineInstance.h"

void tpd::PipelineInstance::setDescriptors(
    const uint32_t instance,
    const uint32_t set,
    const uint32_t binding,
    const vk::DescriptorType type,
    const vk::Device device,
    const std::vector<vk::DescriptorBufferInfo>& bufferInfos) const
{
    auto writeDescriptor = vk::WriteDescriptorSet{};
    writeDescriptor.dstSet = _descriptorSets[instance * _perInstanceSetCount + set];
    writeDescriptor.dstBinding = binding;
    writeDescriptor.dstArrayElement = 0;
    writeDescriptor.descriptorCount = static_cast<uint32_t>(bufferInfos.size());
    writeDescriptor.descriptorType = type;
    writeDescriptor.pBufferInfo = bufferInfos.data();

    device.updateDescriptorSets({ writeDescriptor }, {});
}

void tpd::PipelineInstance::setDescriptors(
    const uint32_t instance,
    const uint32_t set,
    const uint32_t binding,
    const vk::DescriptorType type,
    const vk::Device device,
    const std::vector<vk::DescriptorImageInfo>& imageInfos) const
{
    auto writeDescriptor = vk::WriteDescriptorSet{};
    writeDescriptor.dstSet = _descriptorSets[instance * _perInstanceSetCount + set];
    writeDescriptor.dstBinding = binding;
    writeDescriptor.dstArrayElement = 0;
    writeDescriptor.descriptorCount = static_cast<uint32_t>(imageInfos.size());
    writeDescriptor.descriptorType = type;
    writeDescriptor.pImageInfo = imageInfos.data();

    device.updateDescriptorSets({ writeDescriptor }, {});
}

std::span<const vk::DescriptorSet> tpd::PipelineInstance::getDescriptorSets(const uint32_t instance) const {
    return std::span{ _descriptorSets.begin() + instance * _perInstanceSetCount, _perInstanceSetCount };
}
