#pragma once

#include <vulkan/vulkan.hpp>

#include <span>

namespace tpd {
    class ShaderInstance final {
    public:
        ShaderInstance() = default;

        ShaderInstance(
            uint32_t perInstanceSetCount,
            vk::DescriptorPool descriptorPool,
            std::vector<vk::DescriptorSet>&& descriptorSets) noexcept;

        ShaderInstance(const ShaderInstance&) = delete;
        ShaderInstance& operator=(const ShaderInstance&) = delete;

        [[nodiscard]] std::span<const vk::DescriptorSet> getDescriptorSets(uint32_t instance) const;

        void setDescriptors(
            uint32_t instance, uint32_t setIndex, uint32_t binding, vk::DescriptorType type, vk::Device device,
            const std::vector<vk::DescriptorBufferInfo>& bufferInfos) const;

        void setDescriptors(
            uint32_t instance, uint32_t set, uint32_t binding, vk::DescriptorType type, vk::Device device,
            const std::vector<vk::DescriptorImageInfo>& imageInfos) const;

        [[nodiscard]] bool empty() const;

        void dispose(vk::Device) noexcept;

    private:
        // The number of descriptor sets per instance
        uint32_t _perInstanceSetCount{ 0 };

        vk::DescriptorPool _descriptorPool{};
        std::vector<vk::DescriptorSet> _descriptorSets{};
    };
}

// =========================== //
// INLINE FUNCTION DEFINITIONS //
// =========================== //

inline tpd::ShaderInstance::ShaderInstance(
    const uint32_t perInstanceSetCount, const vk::DescriptorPool descriptorPool,
    std::vector<vk::DescriptorSet>&& descriptorSets) noexcept
: _perInstanceSetCount{ perInstanceSetCount }, _descriptorPool{ descriptorPool }, _descriptorSets{ std::move(descriptorSets) } {
}

inline std::span<const vk::DescriptorSet> tpd::ShaderInstance::getDescriptorSets(const uint32_t instance) const {
    return empty()
        ? std::span<const vk::DescriptorSet>{}
        : std::span{ _descriptorSets.begin() + instance * _perInstanceSetCount, _perInstanceSetCount };
}

inline bool tpd::ShaderInstance::empty() const {
    return _perInstanceSetCount == 0;
}
