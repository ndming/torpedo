#pragma once

#include <vulkan/vulkan.hpp>

#include <span>

namespace tpd {
    class PipelineInstance final {
    public:
        PipelineInstance(
            uint32_t perInstanceSetCount, vk::DescriptorPool descriptorPool,
            std::vector<vk::DescriptorSet>&& descriptorSets) noexcept;

        void setDescriptors(
            uint32_t instance, uint32_t set, uint32_t binding, vk::DescriptorType type, vk::Device device,
            const std::vector<vk::DescriptorBufferInfo>& bufferInfos) const;

        void setDescriptors(
            uint32_t instance, uint32_t set, uint32_t binding, vk::DescriptorType type, vk::Device device,
            const std::vector<vk::DescriptorImageInfo>& imageInfos) const;

        [[nodiscard]] std::span<const vk::DescriptorSet> getDescriptorSets(uint32_t instance) const;

        void destroy(vk::Device) const noexcept;

    private:
        uint32_t _perInstanceSetCount;

        vk::DescriptorPool _descriptorPool;
        std::vector<vk::DescriptorSet> _descriptorSets;
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::PipelineInstance::PipelineInstance(
    const uint32_t perInstanceSetCount, const vk::DescriptorPool descriptorPool,
    std::vector<vk::DescriptorSet>&& descriptorSets) noexcept
: _perInstanceSetCount{ perInstanceSetCount }, _descriptorPool{ descriptorPool }, _descriptorSets{ std::move(descriptorSets) } {
}

inline void tpd::PipelineInstance::destroy(const vk::Device device) const noexcept {
    device.destroyDescriptorPool(_descriptorPool);
}
