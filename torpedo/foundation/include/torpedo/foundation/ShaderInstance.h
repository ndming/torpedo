#pragma once

#include <vulkan/vulkan.hpp>

namespace tpd {
    class ShaderInstance final {
    public:
        /**
        * @brief Default constructor for ShaderInstance.
        * 
        * Constructs an empty ShaderInstance. The notion of emptiness help unify 
        * managing pipelines involving shaders with no descriptor sets.
        */
        ShaderInstance() noexcept = default;

        ShaderInstance(
            vk::DescriptorPool descriptorPool,
            uint32_t setCountPerInstance,
            std::pmr::vector<VkDescriptorSet>&& descriptorSets) noexcept;

        ShaderInstance(const ShaderInstance&) = delete;
        ShaderInstance& operator=(const ShaderInstance&) = delete;

        [[nodiscard]] std::span<const VkDescriptorSet> getDescriptorSets(uint32_t instance) const noexcept;

        void setDescriptors(
            uint32_t instance, uint32_t setIndex, uint32_t binding, vk::DescriptorType type, vk::Device device,
            const std::vector<vk::DescriptorBufferInfo>& bufferInfos) const;

        void setDescriptors(
            uint32_t instance, uint32_t setIndex, uint32_t binding, vk::DescriptorType type, vk::Device device,
            const std::vector<vk::DescriptorImageInfo>& imageInfos) const;

        [[nodiscard]] bool empty() const noexcept;

        void destroy(vk::Device device) noexcept;

    private:
        vk::DescriptorPool _descriptorPool{};
        
        // The number of descriptor sets per instance
        uint32_t _setCountPerInstance{ 0 };
        // Descriptor sets for all instances are flattened into a single vector
        std::pmr::vector<VkDescriptorSet> _descriptorSets{};
    };
}  // namespace tpd

inline tpd::ShaderInstance::ShaderInstance(
    const vk::DescriptorPool descriptorPool, 
    const uint32_t setCountPerInstance,
    std::pmr::vector<VkDescriptorSet>&& descriptorSets) noexcept
    : _descriptorPool{ descriptorPool }
    , _setCountPerInstance{ setCountPerInstance }
    , _descriptorSets{ std::move(descriptorSets) } {
}

inline std::span<const VkDescriptorSet> tpd::ShaderInstance::getDescriptorSets(const uint32_t instance) const noexcept {
    return empty()
        ? std::span<const VkDescriptorSet>{}
        : std::span{ _descriptorSets.begin() + instance * _setCountPerInstance, _setCountPerInstance };
}

inline bool tpd::ShaderInstance::empty() const noexcept {
    return _setCountPerInstance == 0;
}

inline void tpd::ShaderInstance::destroy(const vk::Device device) noexcept {
    device.destroyDescriptorPool(_descriptorPool);
    _descriptorSets.clear();
}
