#pragma once

#include <vulkan/vulkan.hpp>

namespace tpd {
    template<uint32_t SetCount = 0>
    class ShaderInstance final {
    public:
        /**
        * @brief Default constructor for ShaderInstance.
        * 
        * Constructs an empty ShaderInstance. This concept of "emptiness" helps unify the management of pipelines
        * that involve shaders without descriptor sets.
        */
        ShaderInstance() noexcept = default;
        ShaderInstance(vk::DescriptorPool descriptorPool, const std::vector<vk::DescriptorSet>& descriptorSets) noexcept;

        ShaderInstance(ShaderInstance&&) noexcept = default;
        ShaderInstance& operator=(ShaderInstance&&) noexcept = default;

        [[nodiscard]] std::span<const vk::DescriptorSet> getDescriptorSets() const noexcept;

        void setDescriptor(
            uint32_t set, uint32_t binding, vk::DescriptorType type, vk::Device device,
            const std::vector<vk::DescriptorBufferInfo>& bufferInfos) const noexcept;

        void setDescriptor(
            uint32_t set, uint32_t binding, vk::DescriptorType type, vk::Device device,
            const vk::DescriptorBufferInfo& bufferInfo) const noexcept;

        void setDescriptor(
            uint32_t set, uint32_t binding, vk::DescriptorType type, vk::Device device,
            const std::vector<vk::DescriptorImageInfo>& imageInfos) const noexcept;
        
        void setDescriptor(
            uint32_t set, uint32_t binding, vk::DescriptorType type, vk::Device device,
            const vk::DescriptorImageInfo& imageInfo) const noexcept;

        void destroy(vk::Device device) const noexcept;

    private:
        /*------------------*/

        vk::DescriptorPool _descriptorPool{};
        std::array<vk::DescriptorSet, SetCount> _descriptorSets{};

        /*------------------*/
    };
} // namespace tpd

template<uint32_t SetCount>
tpd::ShaderInstance<SetCount>::ShaderInstance(
    const vk::DescriptorPool descriptorPool,
    const std::vector<vk::DescriptorSet>& descriptorSets) noexcept
    : _descriptorPool{ descriptorPool }
{
    std::ranges::copy_n(descriptorSets.begin(), SetCount, _descriptorSets.begin());
}

template<uint32_t SetCount>
std::span<const vk::DescriptorSet> tpd::ShaderInstance<SetCount>::getDescriptorSets() const noexcept {
    return { _descriptorSets.begin(), SetCount };
}

template<uint32_t SetCount>
void tpd::ShaderInstance<SetCount>::setDescriptor(
    const uint32_t set,
    const uint32_t binding,
    const vk::DescriptorType type,
    const vk::Device device,
    const std::vector<vk::DescriptorBufferInfo>& bufferInfos) const noexcept
{
    auto writeDescriptor = vk::WriteDescriptorSet{};
    writeDescriptor.dstSet = _descriptorSets[set];
    writeDescriptor.dstBinding = binding;
    writeDescriptor.dstArrayElement = 0;
    writeDescriptor.descriptorCount = static_cast<uint32_t>(bufferInfos.size());
    writeDescriptor.descriptorType = type;
    writeDescriptor.pBufferInfo = bufferInfos.data();

    device.updateDescriptorSets({ writeDescriptor }, {});
}

template<uint32_t SetCount>
void tpd::ShaderInstance<SetCount>::setDescriptor(
    const uint32_t set,
    const uint32_t binding,
    const vk::DescriptorType type,
    const vk::Device device,
    const vk::DescriptorBufferInfo& bufferInfo) const noexcept
{
    setDescriptor(set, binding, type, device, std::vector{ bufferInfo });
}

template<uint32_t SetCount>
void tpd::ShaderInstance<SetCount>::setDescriptor(
    const uint32_t set,
    const uint32_t binding,
    const vk::DescriptorType type,
    const vk::Device device, const std::vector<vk::DescriptorImageInfo>& imageInfos) const noexcept
{
    auto writeDescriptor = vk::WriteDescriptorSet{};
    writeDescriptor.dstSet = _descriptorSets[set];
    writeDescriptor.dstBinding = binding;
    writeDescriptor.dstArrayElement = 0;
    writeDescriptor.descriptorCount = static_cast<uint32_t>(imageInfos.size());
    writeDescriptor.descriptorType = type;
    writeDescriptor.pImageInfo = imageInfos.data();

    device.updateDescriptorSets({ writeDescriptor }, {});
}

template<uint32_t SetCount>
void tpd::ShaderInstance<SetCount>::setDescriptor(
    const uint32_t set,
    const uint32_t binding,
    const vk::DescriptorType type,
    const vk::Device device,
    const vk::DescriptorImageInfo& imageInfo) const noexcept
{
    setDescriptor(set, binding, type, device, std::vector{ imageInfo });
}

template<uint32_t SetCount>
void tpd::ShaderInstance<SetCount>::destroy(const vk::Device device) const noexcept {
    device.destroyDescriptorPool(_descriptorPool);
}
