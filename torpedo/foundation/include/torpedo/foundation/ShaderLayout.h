#pragma once

#include "torpedo/foundation/ShaderInstance.h"
#include "torpedo/foundation/AllocationUtils.h"

namespace tpd {
    class ShaderLayout final {
    public:
        class Builder {
        public:
            explicit Builder(std::pmr::memory_resource* pool = std::pmr::get_default_resource()) : _pool{ pool } {}

            Builder& descriptorSetCount(uint32_t count);

            Builder& descriptor(
                uint32_t set,
                uint32_t binding,
                vk::DescriptorType type,
                uint32_t elementCount,
                vk::ShaderStageFlags shaderStages,
                vk::DescriptorBindingFlags flags = {});

            Builder& pushConstantRange(
                vk::ShaderStageFlags shaderStages,
                uint32_t byteOffset,
                uint32_t byteSize);

            [[nodiscard]] ShaderLayout build(vk::Device device);
            [[nodiscard]] std::unique_ptr<ShaderLayout, Deleter<ShaderLayout>> buildUnique(vk::Device device);

        private:
            [[nodiscard]] std::pmr::vector<vk::DescriptorSetLayout> createLayouts(vk::Device device);
            [[nodiscard]] vk::PipelineLayout createPipelineLayout(vk::Device device, const std::pmr::vector<vk::DescriptorSetLayout>& layouts);
            
            std::pmr::memory_resource* _pool;
            std::pmr::vector<std::pmr::vector<vk::DescriptorSetLayoutBinding>> _descriptorSetLayoutBindingLists{ _pool };

            std::vector<std::vector<vk::DescriptorBindingFlags>> _descriptorSetBindingFlagLists{};
            std::vector<vk::PushConstantRange> _pushConstantRanges{};
        };

        ShaderLayout(
            vk::PipelineLayout pipelineLayout,
            std::pmr::vector<vk::DescriptorSetLayout>&& descriptorSetLayouts,
            std::pmr::vector<std::pmr::vector<vk::DescriptorSetLayoutBinding>>&& descriptorSetBindingFlagLists) noexcept;

        ShaderLayout(const ShaderLayout&) = delete;
        ShaderLayout& operator=(const ShaderLayout&) = delete;

        [[nodiscard]] ShaderInstance createInstance(
            vk::Device device,
            uint32_t instanceCount = 1,
            uint32_t firstSetIndex = 0,
            vk::DescriptorPoolCreateFlags flags = {}) const;

        [[nodiscard]] std::unique_ptr<ShaderInstance, Deleter<ShaderInstance>> createInstance(
            std::pmr::memory_resource* pool,
            vk::Device device,
            uint32_t instanceCount = 1,
            uint32_t firstSetIndex = 0,
            vk::DescriptorPoolCreateFlags flags = {}) const;

        [[nodiscard]] vk::PipelineLayout getPipelineLayout() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        void destroy(vk::Device device) noexcept;

    private:
        [[nodiscard]] vk::DescriptorPool createDescriptorPool(
            vk::Device device,
            uint32_t instanceCount,
            uint32_t firstSetIndex,
            vk::DescriptorPoolCreateFlags flags) const;

        [[nodiscard]] std::pmr::vector<VkDescriptorSet> createDescriptorSets(
            vk::Device device,
            vk::DescriptorPool descriptorPool,
            uint32_t firstSetIndex,
            uint32_t instanceCount,
            std::pmr::memory_resource* pool = std::pmr::get_default_resource()) const;

        vk::PipelineLayout _pipelineLayout;
        std::pmr::vector<vk::DescriptorSetLayout> _descriptorSetLayouts;
        std::pmr::vector<std::pmr::vector<vk::DescriptorSetLayoutBinding>> _descriptorSetLayoutBindingLists;
    };
}  // namespace tpd

inline tpd::ShaderLayout::Builder& tpd::ShaderLayout::Builder::descriptorSetCount(const uint32_t count) {
    _descriptorSetLayoutBindingLists.resize(count);
    _descriptorSetBindingFlagLists.resize(count);
    return *this;
}

inline tpd::ShaderLayout::Builder& tpd::ShaderLayout::Builder::descriptor(
    const uint32_t set,
    const uint32_t binding,
    const vk::DescriptorType type,
    const uint32_t elementCount,
    const vk::ShaderStageFlags shaderStages,
    const vk::DescriptorBindingFlags flags)
{
    if (set >= _descriptorSetLayoutBindingLists.size()) {
        throw std::invalid_argument(
            "ShaderLayout::Builder - Descriptor set index out of range: did you forget to call Builder::descriptorSetCount()?");
    }
    _descriptorSetLayoutBindingLists[set].emplace_back(binding, type, elementCount, shaderStages);
    _descriptorSetBindingFlagLists[set].push_back(flags);

    return *this;
}

inline tpd::ShaderLayout::Builder&tpd::ShaderLayout::Builder::pushConstantRange(
    const vk::ShaderStageFlags shaderStages,
    const uint32_t byteOffset,
    const uint32_t byteSize)
{
    _pushConstantRanges.emplace_back(shaderStages, byteOffset, byteSize);
    return *this;
}

inline tpd::ShaderLayout::ShaderLayout(
    const vk::PipelineLayout pipelineLayout,
    std::pmr::vector<vk::DescriptorSetLayout>&& descriptorSetLayouts,
    std::pmr::vector<std::pmr::vector<vk::DescriptorSetLayoutBinding>>&& descriptorSetBindingFlagLists) noexcept
    : _pipelineLayout{ pipelineLayout }
    , _descriptorSetLayouts{ std::move(descriptorSetLayouts) }
    , _descriptorSetLayoutBindingLists{ std::move(descriptorSetBindingFlagLists) } {
}

inline vk::PipelineLayout tpd::ShaderLayout::getPipelineLayout() const noexcept {
    return _pipelineLayout;
}

inline bool tpd::ShaderLayout::empty() const noexcept {
    return _descriptorSetLayouts.empty();
}
