#pragma once

#include <vulkan/vulkan.hpp>

namespace tpd {
    class ShaderInstance final {
    public:
        /**
        * @brief Default constructor for ShaderInstance.
        * 
        * Constructs an empty ShaderInstance. This concept of "emptiness" helps unify the management of pipelines
        * that involve shaders without descriptor sets.
        */
        ShaderInstance() noexcept = default;

        ShaderInstance(
            vk::DescriptorPool descriptorPool,
            uint32_t setCountPerInstance,
            const std::vector<vk::DescriptorSet>& descriptorSets) noexcept;

        ShaderInstance(ShaderInstance&&) noexcept = default;
        ShaderInstance& operator=(ShaderInstance&&) noexcept = default;

        [[nodiscard]] std::span<const vk::DescriptorSet> getDescriptorSets(uint32_t instance) const noexcept;

        void setDescriptor(
            uint32_t instance, uint32_t setIndex, uint32_t binding, vk::DescriptorType type, vk::Device device,
            const std::vector<vk::DescriptorBufferInfo>& bufferInfos) const;

        void setDescriptor(
            uint32_t instance, uint32_t setIndex, uint32_t binding, vk::DescriptorType type, vk::Device device,
            const vk::DescriptorBufferInfo& bufferInfo) const;

        void setDescriptor(
            uint32_t instance, uint32_t setIndex, uint32_t binding, vk::DescriptorType type, vk::Device device,
            const std::vector<vk::DescriptorImageInfo>& imageInfos) const;
        
        void setDescriptor(
            uint32_t instance, uint32_t setIndex, uint32_t binding, vk::DescriptorType type, vk::Device device,
            const vk::DescriptorImageInfo& imageInfo) const;

        [[nodiscard]] bool empty() const noexcept;
        void destroy(vk::Device device) const noexcept;

        static constexpr uint32_t MAX_DESCRIPTOR_SETS = 3;
        static constexpr uint32_t MAX_INSTANCES = 2;

    private:
        /*------------------*/

        vk::DescriptorPool _descriptorPool{};
        uint32_t _setCountPerInstance{ 0 };
        std::array<vk::DescriptorSet, MAX_DESCRIPTOR_SETS * MAX_INSTANCES> _descriptorSets{};

        /*------------------*/
    };
} // namespace tpd

inline tpd::ShaderInstance::ShaderInstance(
    const vk::DescriptorPool descriptorPool, 
    const uint32_t setCountPerInstance,
    const std::vector<vk::DescriptorSet>& descriptorSets) noexcept
    : _descriptorPool{ descriptorPool }
    , _setCountPerInstance{ setCountPerInstance }
{
    std::ranges::copy_n(descriptorSets.begin(), descriptorSets.size(), _descriptorSets.begin());
}

inline std::span<const vk::DescriptorSet> tpd::ShaderInstance::getDescriptorSets(const uint32_t instance) const noexcept {
    return { _descriptorSets.begin() + instance * _setCountPerInstance, _setCountPerInstance };
}

inline bool tpd::ShaderInstance::empty() const noexcept {
    return _setCountPerInstance == 0;
}

inline void tpd::ShaderInstance::destroy(const vk::Device device) const noexcept {
    device.destroyDescriptorPool(_descriptorPool);
}
