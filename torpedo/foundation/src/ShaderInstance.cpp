#include "torpedo/foundation/ShaderInstance.h"

void tpd::ShaderInstance::setDescriptor(
    const uint32_t instance,
    const uint32_t setIndex,
    const uint32_t binding,
    const vk::DescriptorType type,
    const vk::Device device,
    const std::vector<vk::DescriptorBufferInfo>& bufferInfos) const
{
    auto writeDescriptor = vk::WriteDescriptorSet{};
    writeDescriptor.dstSet = _descriptorSets[instance * _setCountPerInstance + setIndex];
    writeDescriptor.dstBinding = binding;
    writeDescriptor.dstArrayElement = 0;
    writeDescriptor.descriptorCount = static_cast<uint32_t>(bufferInfos.size());
    writeDescriptor.descriptorType = type;
    writeDescriptor.pBufferInfo = bufferInfos.data();

    device.updateDescriptorSets({ writeDescriptor }, {});
}

void tpd::ShaderInstance::setDescriptor(
    const uint32_t instance,
    const uint32_t setIndex,
    const uint32_t binding,
    const vk::DescriptorType type,
    const vk::Device device,
    const std::vector<vk::DescriptorImageInfo>& imageInfos) const
{
    auto writeDescriptor = vk::WriteDescriptorSet{};
    writeDescriptor.dstSet = _descriptorSets[instance * _setCountPerInstance + setIndex];
    writeDescriptor.dstBinding = binding;
    writeDescriptor.dstArrayElement = 0;
    writeDescriptor.descriptorCount = static_cast<uint32_t>(imageInfos.size());
    writeDescriptor.descriptorType = type;
    writeDescriptor.pImageInfo = imageInfos.data();

    device.updateDescriptorSets({ writeDescriptor }, {});
}

void tpd::ShaderInstance::setDescriptor(
    const uint32_t instance,
    const uint32_t setIndex,
    const uint32_t binding,
    const vk::DescriptorType type,
    const vk::Device device,
    const vk::DescriptorImageInfo& imageInfo) const
{
    setDescriptor(instance, setIndex, binding, type, device, std::vector{ imageInfo });
}
