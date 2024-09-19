#include "torpedo/foundation/PipelineShader.h"

#include <fstream>
#include <ranges>
#include <unordered_set>

std::unique_ptr<tpd::PipelineShader> tpd::PipelineShader::Builder::build(
    vk::GraphicsPipelineCreateInfo* pipelineInfo,
    const vk::PhysicalDevice physicalDevice,
    const vk::Device device)
{
    // Descriptor layouts and pipeline layout
    checkPushConstants(physicalDevice);
    auto bindingFlagInfo = vk::DescriptorSetLayoutBindingFlagsCreateInfo{};
    bindingFlagInfo.bindingCount = static_cast<uint32_t>(_descriptorBindingFlags.size());
    bindingFlagInfo.pBindingFlags = _descriptorBindingFlags.data();
    auto descriptorSetLayouts = createDescriptorSetLayouts(device);
    auto pipelineLayoutInfo = vk::PipelineLayoutCreateInfo{};
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(_pushConstantRanges.size());
    pipelineLayoutInfo.pPushConstantRanges = _pushConstantRanges.data();
    const auto pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);
    pipelineInfo->layout = pipelineLayout;

    // Shader stages
    const auto shaderModules = createShaderModules(device);
    const auto toShaderStageInfo = [this, &shaderModules](const auto i) {
        auto shaderStageCreateInfo = vk::PipelineShaderStageCreateInfo{};
        shaderStageCreateInfo.stage = _shaderStages[i];
        shaderStageCreateInfo.module = shaderModules[i];
        shaderStageCreateInfo.pName = _shaderEntryPoints[i].c_str();
        return shaderStageCreateInfo;
    };
    const auto shaderStageInfos = std::views::iota(0, static_cast<int>(_shaderCodes.size()))
        | std::views::transform(toShaderStageInfo)
        | std::ranges::to<std::vector>();
    pipelineInfo->stageCount = static_cast<uint32_t>(shaderStageInfos.size());
    pipelineInfo->pStages = shaderStageInfos.data();

    // Create the graphics pipeline
    const auto pipeline = device.createGraphicsPipeline(nullptr, *pipelineInfo).value;
    // We no longer need the shader modules once the pipeline is created
    std::ranges::for_each(shaderModules, [&device](const auto& it) { device.destroyShaderModule(it); });

    return std::make_unique<PipelineShader>(pipelineLayout, pipeline, std::move(descriptorSetLayouts), std::move(_descriptorSetLayoutBindings));
}

std::vector<std::byte> tpd::PipelineShader::Builder::readShaderFile(const std::filesystem::path& path) {
    // Reading from the end of the file as binary format
    auto file = std::ifstream{ path, std::ios::ate | std::ios::binary };
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + path.string());
    }

    // The advantage of starting to read at the end of the file is that we can use the read position to
    // determine the size of the file and allocate a buffer
    const auto fileSize = static_cast<size_t>(file.tellg());
    auto buffer = std::vector<char>(fileSize);

    // Seek back to the beginning of the file and read all the bytes at once
    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(fileSize));

    file.close();
    return buffer
        | std::views::transform([](const auto it) { return static_cast<std::byte>(it); })
        | std::ranges::to<std::vector>();
}

std::vector<vk::ShaderModule> tpd::PipelineShader::Builder::createShaderModules(vk::Device device) const {
    const auto toShaderModule = [&device](const std::vector<std::byte>& shaderCode) {
        auto shaderModuleCreateInfo = vk::ShaderModuleCreateInfo{};
        shaderModuleCreateInfo.codeSize = static_cast<uint32_t>(shaderCode.size());
        shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());
        return device.createShaderModule(shaderModuleCreateInfo);
    };
    using namespace std::ranges;
    return _shaderCodes | views::transform(toShaderModule) | to<std::vector>();
}

std::vector<vk::DescriptorSetLayout> tpd::PipelineShader::Builder::createDescriptorSetLayouts(const vk::Device device) const {
    auto descriptorSetLayouts = std::vector<vk::DescriptorSetLayout>{};
    for (const auto& bindings : _descriptorSetBindingLists) {
        auto descriptorSetBindings = std::vector<vk::DescriptorSetLayoutBinding>{};
        auto descriptorSetBindingFlags = std::vector<vk::DescriptorBindingFlags>{};
        for (const auto binding : bindings) {
            descriptorSetBindings.push_back(_descriptorSetLayoutBindings[binding]);
            descriptorSetBindingFlags.push_back(_descriptorBindingFlags[binding]);
        }

        auto bindingFlagInfo = vk::DescriptorSetLayoutBindingFlagsCreateInfo{};
        bindingFlagInfo.bindingCount = static_cast<uint32_t>(descriptorSetBindingFlags.size());
        bindingFlagInfo.pBindingFlags = descriptorSetBindingFlags.data();

        auto layoutInfo = vk::DescriptorSetLayoutCreateInfo{};
        layoutInfo.bindingCount = static_cast<uint32_t>(descriptorSetBindings.size());
        layoutInfo.pBindings = descriptorSetBindings.data();
        layoutInfo.pNext = &bindingFlagInfo;

        descriptorSetLayouts.push_back(device.createDescriptorSetLayout(layoutInfo));
    }
    return descriptorSetLayouts;
}

template<> struct std::hash<vk::ShaderStageFlags> {
    size_t operator()(const vk::ShaderStageFlags& stageFlags) const noexcept {
        return std::hash<VkFlags>()(static_cast<VkFlags>(stageFlags));
    }
};

bool operator==(const vk::ShaderStageFlags& lhs, const vk::ShaderStageFlags& rhs) {
    return static_cast<VkFlags>(lhs) == static_cast<VkFlags>(rhs);
}

void tpd::PipelineShader::Builder::checkPushConstants(const vk::PhysicalDevice physicalDevice) const {
    // Ensure that the specified push constants are within the device's limit
    const auto psLimit = physicalDevice.getProperties().limits.maxPushConstantsSize;
    const auto exceedLimit = [&psLimit](const vk::PushConstantRange& range) {
        return range.offset + range.size > psLimit;
    };
    if (std::ranges::any_of(_pushConstantRanges, exceedLimit)) {
        throw std::runtime_error(
            "Push constant range (offset + size) must be within the allowed limit: " + std::to_string(psLimit));
    }

    // Each shader stage should only have one push constant declaration
    auto seenFlags = std::unordered_set<vk::ShaderStageFlags>{};
    for (const auto& range : _pushConstantRanges) {
        if (seenFlags.contains(range.stageFlags)) {
            // Duplicate found
            throw std::runtime_error("Each shader stage is only allowed to have one push constant range");
        }
        seenFlags.insert(range.stageFlags);
    }
}

void tpd::PipelineShader::destroy(const vk::Device device) noexcept {
    device.destroyPipelineLayout(_pipelineLayout);
    device.destroyPipeline(_pipeline);
    std::ranges::for_each(_descriptorSetLayouts, [&device](const auto& it) { device.destroyDescriptorSetLayout(it); });
}
