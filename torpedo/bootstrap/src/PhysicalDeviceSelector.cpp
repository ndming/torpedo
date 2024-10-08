#include "torpedo/bootstrap/PhysicalDeviceSelector.h"

#include <ranges>
#include <unordered_set>

tpd::PhysicalDeviceSelector& tpd::PhysicalDeviceSelector::select(const vk::Instance instance) {
    const auto devices = instance.enumeratePhysicalDevices();

    const auto suitable = [this](const auto& device) {
        const auto indices = findQueueFamilies(device);
        const auto extensionSupported = checkExtensionSupport(device);
        const auto featureSupported = checkDeviceFeatures(device);

        auto swapChainAdequate = true;
        if (_requestPresentQueueFamily && extensionSupported) {
            const auto capabilities = device.getSurfaceCapabilitiesKHR(_surface);
            const auto formats = device.getSurfaceFormatsKHR(_surface);
            const auto presentModes = device.getSurfacePresentModesKHR(_surface);
            swapChainAdequate = !formats.empty() && !presentModes.empty();
        }

        return queueFamiliesComplete(indices) && extensionSupported && swapChainAdequate && featureSupported;
    };

    if (const auto found = std::ranges::find_if(devices, suitable); found != devices.end()) {
        _physicalDevice = *found;

        const auto [graphicsFamily, presentFamily, computeFamily] = findQueueFamilies(_physicalDevice);
        if (_requestGraphicsQueueFamily) _graphicsQueueFamily = graphicsFamily.value();
        if (_requestPresentQueueFamily) _presentQueueFamily = presentFamily.value();
        if (_requestComputeQueueFamily) _computeQueueFamily = computeFamily.value();
    } else {
        throw std::runtime_error(
            "PhysicalDeviceSelector: failed to find a suitable device, consider requesting less extensions and features");
    }

    _isSelected = true;
    return *this;
}

bool tpd::PhysicalDeviceSelector::checkExtensionSupport(const vk::PhysicalDevice& device) const {
    using namespace std::ranges;
    const auto availableExtensions = device.enumerateDeviceExtensionProperties();
    const auto toName = [](const vk::ExtensionProperties& extension) { return extension.extensionName.data(); };
    const auto extensions = availableExtensions | views::transform(toName) | to<std::unordered_set<std::string>>();

    const auto available = [&extensions](const auto& it) { return extensions.contains(it); };
    return all_of(_deviceExtensions, std::identity{}, available);
}

tpd::PhysicalDeviceSelector::QueueFamilyIndices tpd::PhysicalDeviceSelector::findQueueFamilies(const vk::PhysicalDevice& device) const {
    const auto queueFamilies = device.getQueueFamilyProperties();
    auto indices = QueueFamilyIndices{};

    for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
        if (!indices.graphicsFamily.has_value() && _requestGraphicsQueueFamily &&
            queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.graphicsFamily = i;
        }

        if (!indices.computeFamily.has_value() && _requestComputeQueueFamily &&
            queueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute) {
            indices.computeFamily = i;
        }

        if (!indices.presentFamily.has_value() && _requestPresentQueueFamily &&
            device.getSurfaceSupportKHR(i, _surface)) {
            indices.presentFamily = i;
        }

        if (_requestGraphicsQueueFamily && _requestComputeQueueFamily && _distinctComputeQueueFamily &&
            indices.graphicsFamily.has_value() && indices.computeFamily.has_value() &&
            indices.graphicsFamily.value() == indices.computeFamily.value()) {
            indices.computeFamily.reset();
        }

        if (queueFamiliesComplete(indices)) {
            break;
        }
    }

    return indices;
}

bool tpd::PhysicalDeviceSelector::queueFamiliesComplete(const QueueFamilyIndices& indices) const {
    if (_requestGraphicsQueueFamily && !indices.graphicsFamily.has_value()) return false;
    if (_requestPresentQueueFamily && !indices.presentFamily.has_value())   return false;
    if (_requestComputeQueueFamily && !indices.computeFamily.has_value())   return false;
    if (_distinctComputeQueueFamily && indices.graphicsFamily == indices.computeFamily) return false;
    return true;
}

bool tpd::PhysicalDeviceSelector::checkDeviceFeatures(const vk::PhysicalDevice& device) const {
    auto features = vk::PhysicalDeviceFeatures{};
    device.getFeatures(&features);

    auto extendedDynamicState1Features = vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT{};
    auto extendedDynamicState2Features = vk::PhysicalDeviceExtendedDynamicState2FeaturesEXT{};
    auto extendedDynamicState3Features = vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT{};
    auto vulkan11Features = vk::PhysicalDeviceVulkan11Features{};
    auto vulkan12Features = vk::PhysicalDeviceVulkan12Features{};
    auto vulkan13Features = vk::PhysicalDeviceVulkan13Features{};
    auto conditionalRenderingFeatures = vk::PhysicalDeviceConditionalRenderingFeaturesEXT{};
    auto vertexInputDynamicStateFeatures = vk::PhysicalDeviceVertexInputDynamicStateFeaturesEXT{};

    auto supportedFeatures = vk::PhysicalDeviceFeatures2{};
    supportedFeatures.features = features;
    supportedFeatures.pNext = &extendedDynamicState1Features;
    extendedDynamicState1Features.pNext = &extendedDynamicState2Features;
    extendedDynamicState2Features.pNext = &extendedDynamicState3Features;
    extendedDynamicState3Features.pNext = &vulkan11Features;
    vulkan11Features.pNext = &vulkan12Features;
    vulkan12Features.pNext = &vulkan13Features;
    vulkan13Features.pNext = &conditionalRenderingFeatures;
    conditionalRenderingFeatures.pNext = &vertexInputDynamicStateFeatures;

    device.getFeatures2(&supportedFeatures);

    return checkFeatures(features) &&
        checkExtendedDynamicState1Features(extendedDynamicState1Features) &&
        checkExtendedDynamicState2Features(extendedDynamicState2Features) &&
        checkExtendedDynamicState3Features(extendedDynamicState3Features) &&
        checkVulkan11Features(vulkan11Features) &&
        checkVulkan12Features(vulkan12Features) &&
        checkVulkan13Features(vulkan13Features) &&
        checkConditionalRenderingFeatures(conditionalRenderingFeatures) &&
        checkVertexInputDynamicStateFeatures(vertexInputDynamicStateFeatures);
}

bool tpd::PhysicalDeviceSelector::checkFeatures(const vk::PhysicalDeviceFeatures& features) const {
    if (_features.robustBufferAccess && !features.robustBufferAccess) return false;
    if (_features.fullDrawIndexUint32 && !features.fullDrawIndexUint32) return false;
    if (_features.imageCubeArray && !features.imageCubeArray) return false;
    if (_features.independentBlend && !features.independentBlend) return false;
    if (_features.geometryShader && !features.geometryShader) return false;
    if (_features.tessellationShader && !features.tessellationShader) return false;
    if (_features.sampleRateShading && !features.sampleRateShading) return false;
    if (_features.dualSrcBlend && !features.dualSrcBlend) return false;
    if (_features.logicOp && !features.logicOp) return false;
    if (_features.multiDrawIndirect && !features.multiDrawIndirect) return false;
    if (_features.drawIndirectFirstInstance && !features.drawIndirectFirstInstance) return false;
    if (_features.depthClamp && !features.depthClamp) return false;
    if (_features.depthBiasClamp && !features.depthBiasClamp) return false;
    if (_features.fillModeNonSolid && !features.fillModeNonSolid) return false;
    if (_features.depthBounds && !features.depthBounds) return false;
    if (_features.wideLines && !features.wideLines) return false;
    if (_features.largePoints && !features.largePoints) return false;
    if (_features.alphaToOne && !features.alphaToOne) return false;
    if (_features.multiViewport && !features.multiViewport) return false;
    if (_features.samplerAnisotropy && !features.samplerAnisotropy) return false;
    if (_features.textureCompressionETC2 && !features.textureCompressionETC2) return false;
    if (_features.textureCompressionASTC_LDR && !features.textureCompressionASTC_LDR) return false;
    if (_features.textureCompressionBC && !features.textureCompressionBC) return false;
    if (_features.occlusionQueryPrecise && !features.occlusionQueryPrecise) return false;
    if (_features.pipelineStatisticsQuery && !features.pipelineStatisticsQuery) return false;
    if (_features.vertexPipelineStoresAndAtomics && !features.vertexPipelineStoresAndAtomics) return false;
    if (_features.fragmentStoresAndAtomics && !features.fragmentStoresAndAtomics) return false;
    if (_features.shaderTessellationAndGeometryPointSize && !features.shaderTessellationAndGeometryPointSize) return false;
    if (_features.shaderImageGatherExtended && !features.shaderImageGatherExtended) return false;
    if (_features.shaderStorageImageExtendedFormats && !features.shaderStorageImageExtendedFormats) return false;
    if (_features.shaderStorageImageMultisample && !features.shaderStorageImageMultisample) return false;
    if (_features.shaderStorageImageReadWithoutFormat && !features.shaderStorageImageReadWithoutFormat) return false;
    if (_features.shaderStorageImageWriteWithoutFormat && !features.shaderStorageImageWriteWithoutFormat) return false;
    if (_features.shaderUniformBufferArrayDynamicIndexing && !features.shaderUniformBufferArrayDynamicIndexing) return false;
    if (_features.shaderSampledImageArrayDynamicIndexing && !features.shaderSampledImageArrayDynamicIndexing) return false;
    if (_features.shaderStorageBufferArrayDynamicIndexing && !features.shaderStorageBufferArrayDynamicIndexing) return false;
    if (_features.shaderStorageImageArrayDynamicIndexing && !features.shaderStorageImageArrayDynamicIndexing) return false;
    if (_features.shaderClipDistance && !features.shaderClipDistance) return false;
    if (_features.shaderCullDistance && !features.shaderCullDistance) return false;
    if (_features.shaderFloat64 && !features.shaderFloat64) return false;
    if (_features.shaderInt64 && !features.shaderInt64) return false;
    if (_features.shaderInt16 && !features.shaderInt16) return false;
    if (_features.shaderResourceResidency && !features.shaderResourceResidency) return false;
    if (_features.shaderResourceMinLod && !features.shaderResourceMinLod) return false;
    if (_features.sparseBinding && !features.sparseBinding) return false;
    if (_features.sparseResidencyBuffer && !features.sparseResidencyBuffer) return false;
    if (_features.sparseResidencyImage2D && !features.sparseResidencyImage2D) return false;
    if (_features.sparseResidencyImage3D && !features.sparseResidencyImage3D) return false;
    if (_features.sparseResidency2Samples && !features.sparseResidency2Samples) return false;
    if (_features.sparseResidency4Samples && !features.sparseResidency4Samples) return false;
    if (_features.sparseResidency8Samples && !features.sparseResidency8Samples) return false;
    if (_features.sparseResidency16Samples && !features.sparseResidency16Samples) return false;
    if (_features.sparseResidencyAliased && !features.sparseResidencyAliased) return false;
    if (_features.variableMultisampleRate && !features.variableMultisampleRate) return false;
    if (_features.inheritedQueries && !features.inheritedQueries) return false;
    return true;
}

bool tpd::PhysicalDeviceSelector::checkExtendedDynamicState1Features(const vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT& features) const {
    if (_extendedDynamicState1Features.extendedDynamicState && !features.extendedDynamicState) return false;
    return true;
}

bool tpd::PhysicalDeviceSelector::checkExtendedDynamicState2Features(const vk::PhysicalDeviceExtendedDynamicState2FeaturesEXT& features) const {
    if (_extendedDynamicState2Features.extendedDynamicState2 && !features.extendedDynamicState2) return false;
    if (_extendedDynamicState2Features.extendedDynamicState2LogicOp && !features.extendedDynamicState2LogicOp) return false;
    if (_extendedDynamicState2Features.extendedDynamicState2PatchControlPoints && !features.extendedDynamicState2PatchControlPoints) return false;
    return true;
}

bool tpd::PhysicalDeviceSelector::checkExtendedDynamicState3Features(const vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT& features) const {
    if (_extendedDynamicState3Features.extendedDynamicState3TessellationDomainOrigin && !features.extendedDynamicState3TessellationDomainOrigin) return false;
    if (_extendedDynamicState3Features.extendedDynamicState3DepthClampEnable && !features.extendedDynamicState3DepthClampEnable) return false;
    if (_extendedDynamicState3Features.extendedDynamicState3PolygonMode && !features.extendedDynamicState3PolygonMode) return false;
    if (_extendedDynamicState3Features.extendedDynamicState3RasterizationSamples && !features.extendedDynamicState3RasterizationSamples) return false;
    if (_extendedDynamicState3Features.extendedDynamicState3SampleMask && !features.extendedDynamicState3SampleMask) return false;
    if (_extendedDynamicState3Features.extendedDynamicState3AlphaToCoverageEnable && !features.extendedDynamicState3AlphaToCoverageEnable) return false;
    if (_extendedDynamicState3Features.extendedDynamicState3AlphaToOneEnable && !features.extendedDynamicState3AlphaToOneEnable) return false;
    if (_extendedDynamicState3Features.extendedDynamicState3LogicOpEnable && !features.extendedDynamicState3LogicOpEnable) return false;
    if (_extendedDynamicState3Features.extendedDynamicState3ColorBlendEnable && !features.extendedDynamicState3ColorBlendEnable) return false;
    if (_extendedDynamicState3Features.extendedDynamicState3ColorBlendEquation && !features.extendedDynamicState3ColorBlendEquation) return false;
    if (_extendedDynamicState3Features.extendedDynamicState3ColorWriteMask && !features.extendedDynamicState3ColorWriteMask) return false;
    if (_extendedDynamicState3Features.extendedDynamicState3RasterizationStream && !features.extendedDynamicState3RasterizationStream) return false;
    if (_extendedDynamicState3Features.extendedDynamicState3ConservativeRasterizationMode && !features.extendedDynamicState3ConservativeRasterizationMode) return false;
    if (_extendedDynamicState3Features.extendedDynamicState3ExtraPrimitiveOverestimationSize && !features.extendedDynamicState3ExtraPrimitiveOverestimationSize) return false;
    if (_extendedDynamicState3Features.extendedDynamicState3DepthClipEnable && !features.extendedDynamicState3DepthClipEnable) return false;
    if (_extendedDynamicState3Features.extendedDynamicState3SampleLocationsEnable && !features.extendedDynamicState3SampleLocationsEnable) return false;
    if (_extendedDynamicState3Features.extendedDynamicState3ColorBlendAdvanced && !features.extendedDynamicState3ColorBlendAdvanced) return false;
    if (_extendedDynamicState3Features.extendedDynamicState3ProvokingVertexMode && !features.extendedDynamicState3ProvokingVertexMode) return false;
    if (_extendedDynamicState3Features.extendedDynamicState3LineRasterizationMode && !features.extendedDynamicState3LineRasterizationMode) return false;
    if (_extendedDynamicState3Features.extendedDynamicState3LineStippleEnable && !features.extendedDynamicState3LineStippleEnable) return false;
    if (_extendedDynamicState3Features.extendedDynamicState3DepthClipNegativeOneToOne && !features.extendedDynamicState3DepthClipNegativeOneToOne) return false;
    if (_extendedDynamicState3Features.extendedDynamicState3ViewportWScalingEnable && !features.extendedDynamicState3ViewportWScalingEnable) return false;
    if (_extendedDynamicState3Features.extendedDynamicState3ViewportSwizzle && !features.extendedDynamicState3ViewportSwizzle) return false;
    if (_extendedDynamicState3Features.extendedDynamicState3CoverageToColorEnable && !features.extendedDynamicState3CoverageToColorEnable) return false;
    if (_extendedDynamicState3Features.extendedDynamicState3CoverageToColorLocation && !features.extendedDynamicState3CoverageToColorLocation) return false;
    if (_extendedDynamicState3Features.extendedDynamicState3CoverageModulationMode && !features.extendedDynamicState3CoverageModulationMode) return false;
    if (_extendedDynamicState3Features.extendedDynamicState3CoverageModulationTableEnable && !features.extendedDynamicState3CoverageModulationTableEnable) return false;
    if (_extendedDynamicState3Features.extendedDynamicState3CoverageModulationTable && !features.extendedDynamicState3CoverageModulationTable) return false;
    if (_extendedDynamicState3Features.extendedDynamicState3CoverageReductionMode && !features.extendedDynamicState3CoverageReductionMode) return false;
    if (_extendedDynamicState3Features.extendedDynamicState3RepresentativeFragmentTestEnable && !features.extendedDynamicState3RepresentativeFragmentTestEnable) return false;
    if (_extendedDynamicState3Features.extendedDynamicState3ShadingRateImageEnable && !features.extendedDynamicState3ShadingRateImageEnable) return false;
    return true;
}

bool tpd::PhysicalDeviceSelector::checkVulkan11Features(const vk::PhysicalDeviceVulkan11Features& features) const {
    if (_vulkan11Features.storageBuffer16BitAccess && !features.storageBuffer16BitAccess) return false;
    if (_vulkan11Features.uniformAndStorageBuffer16BitAccess && !features.uniformAndStorageBuffer16BitAccess) return false;
    if (_vulkan11Features.storagePushConstant16 && !features.storagePushConstant16) return false;
    if (_vulkan11Features.storageInputOutput16 && !features.storageInputOutput16) return false;
    if (_vulkan11Features.multiview && !features.multiview) return false;
    if (_vulkan11Features.multiviewGeometryShader && !features.multiviewGeometryShader) return false;
    if (_vulkan11Features.multiviewTessellationShader && !features.multiviewTessellationShader) return false;
    if (_vulkan11Features.variablePointersStorageBuffer && !features.variablePointersStorageBuffer) return false;
    if (_vulkan11Features.variablePointers && !features.variablePointers) return false;
    if (_vulkan11Features.protectedMemory && !features.protectedMemory) return false;
    if (_vulkan11Features.samplerYcbcrConversion && !features.samplerYcbcrConversion) return false;
    if (_vulkan11Features.shaderDrawParameters && !features.shaderDrawParameters) return false;
    return true;
}

bool tpd::PhysicalDeviceSelector::checkVulkan12Features(const vk::PhysicalDeviceVulkan12Features& features) const {
    if (_vulkan12Features.samplerMirrorClampToEdge && !features.samplerMirrorClampToEdge) return false;
    if (_vulkan12Features.drawIndirectCount && !features.drawIndirectCount) return false;
    if (_vulkan12Features.storageBuffer8BitAccess && !features.storageBuffer8BitAccess) return false;
    if (_vulkan12Features.uniformAndStorageBuffer8BitAccess && !features.uniformAndStorageBuffer8BitAccess) return false;
    if (_vulkan12Features.storagePushConstant8 && !features.storagePushConstant8) return false;
    if (_vulkan12Features.shaderBufferInt64Atomics && !features.shaderBufferInt64Atomics) return false;
    if (_vulkan12Features.shaderSharedInt64Atomics && !features.shaderSharedInt64Atomics) return false;
    if (_vulkan12Features.shaderFloat16 && !features.shaderFloat16) return false;
    if (_vulkan12Features.shaderInt8 && !features.shaderInt8) return false;
    if (_vulkan12Features.descriptorIndexing && !features.descriptorIndexing) return false;
    if (_vulkan12Features.shaderInputAttachmentArrayDynamicIndexing && !features.shaderInputAttachmentArrayDynamicIndexing) return false;
    if (_vulkan12Features.shaderUniformTexelBufferArrayDynamicIndexing && !features.shaderUniformTexelBufferArrayDynamicIndexing) return false;
    if (_vulkan12Features.shaderStorageTexelBufferArrayDynamicIndexing && !features.shaderStorageTexelBufferArrayDynamicIndexing) return false;
    if (_vulkan12Features.shaderUniformBufferArrayNonUniformIndexing && !features.shaderUniformBufferArrayNonUniformIndexing) return false;
    if (_vulkan12Features.shaderSampledImageArrayNonUniformIndexing && !features.shaderSampledImageArrayNonUniformIndexing) return false;
    if (_vulkan12Features.shaderStorageBufferArrayNonUniformIndexing && !features.shaderStorageBufferArrayNonUniformIndexing) return false;
    if (_vulkan12Features.shaderStorageImageArrayNonUniformIndexing && !features.shaderStorageImageArrayNonUniformIndexing) return false;
    if (_vulkan12Features.shaderInputAttachmentArrayNonUniformIndexing && !features.shaderInputAttachmentArrayNonUniformIndexing) return false;
    if (_vulkan12Features.shaderUniformTexelBufferArrayNonUniformIndexing && !features.shaderUniformTexelBufferArrayNonUniformIndexing) return false;
    if (_vulkan12Features.shaderStorageTexelBufferArrayNonUniformIndexing && !features.shaderStorageTexelBufferArrayNonUniformIndexing) return false;
    if (_vulkan12Features.descriptorBindingUniformBufferUpdateAfterBind && !features.descriptorBindingUniformBufferUpdateAfterBind) return false;
    if (_vulkan12Features.descriptorBindingSampledImageUpdateAfterBind && !features.descriptorBindingSampledImageUpdateAfterBind) return false;
    if (_vulkan12Features.descriptorBindingStorageImageUpdateAfterBind && !features.descriptorBindingStorageImageUpdateAfterBind) return false;
    if (_vulkan12Features.descriptorBindingStorageBufferUpdateAfterBind && !features.descriptorBindingStorageBufferUpdateAfterBind) return false;
    if (_vulkan12Features.descriptorBindingUniformTexelBufferUpdateAfterBind && !features.descriptorBindingUniformTexelBufferUpdateAfterBind) return false;
    if (_vulkan12Features.descriptorBindingStorageTexelBufferUpdateAfterBind && !features.descriptorBindingStorageTexelBufferUpdateAfterBind) return false;
    if (_vulkan12Features.descriptorBindingUpdateUnusedWhilePending && !features.descriptorBindingUpdateUnusedWhilePending) return false;
    if (_vulkan12Features.descriptorBindingPartiallyBound && !features.descriptorBindingPartiallyBound) return false;
    if (_vulkan12Features.descriptorBindingVariableDescriptorCount && !features.descriptorBindingVariableDescriptorCount) return false;
    if (_vulkan12Features.runtimeDescriptorArray && !features.runtimeDescriptorArray) return false;
    if (_vulkan12Features.samplerFilterMinmax && !features.samplerFilterMinmax) return false;
    if (_vulkan12Features.scalarBlockLayout && !features.scalarBlockLayout) return false;
    if (_vulkan12Features.imagelessFramebuffer && !features.imagelessFramebuffer) return false;
    if (_vulkan12Features.uniformBufferStandardLayout && !features.uniformBufferStandardLayout) return false;
    if (_vulkan12Features.shaderSubgroupExtendedTypes && !features.shaderSubgroupExtendedTypes) return false;
    if (_vulkan12Features.separateDepthStencilLayouts && !features.separateDepthStencilLayouts) return false;
    if (_vulkan12Features.hostQueryReset && !features.hostQueryReset) return false;
    if (_vulkan12Features.timelineSemaphore && !features.timelineSemaphore) return false;
    if (_vulkan12Features.bufferDeviceAddress && !features.bufferDeviceAddress) return false;
    if (_vulkan12Features.bufferDeviceAddressCaptureReplay && !features.bufferDeviceAddressCaptureReplay) return false;
    if (_vulkan12Features.bufferDeviceAddressMultiDevice && !features.bufferDeviceAddressMultiDevice) return false;
    if (_vulkan12Features.vulkanMemoryModel && !features.vulkanMemoryModel) return false;
    if (_vulkan12Features.vulkanMemoryModelDeviceScope && !features.vulkanMemoryModelDeviceScope) return false;
    if (_vulkan12Features.vulkanMemoryModelAvailabilityVisibilityChains && !features.vulkanMemoryModelAvailabilityVisibilityChains) return false;
    if (_vulkan12Features.shaderOutputViewportIndex && !features.shaderOutputViewportIndex) return false;
    if (_vulkan12Features.shaderOutputLayer && !features.shaderOutputLayer) return false;
    if (_vulkan12Features.subgroupBroadcastDynamicId && !features.subgroupBroadcastDynamicId) return false;
    return true;
}

bool tpd::PhysicalDeviceSelector::checkVulkan13Features(const vk::PhysicalDeviceVulkan13Features& features) const {
    if (_vulkan13Features.robustImageAccess && !features.robustImageAccess) return false;
    if (_vulkan13Features.inlineUniformBlock && !features.inlineUniformBlock) return false;
    if (_vulkan13Features.descriptorBindingInlineUniformBlockUpdateAfterBind && !features.descriptorBindingInlineUniformBlockUpdateAfterBind) return false;
    if (_vulkan13Features.pipelineCreationCacheControl && !features.pipelineCreationCacheControl) return false;
    if (_vulkan13Features.privateData && !features.privateData) return false;
    if (_vulkan13Features.shaderDemoteToHelperInvocation && !features.shaderDemoteToHelperInvocation) return false;
    if (_vulkan13Features.shaderTerminateInvocation && !features.shaderTerminateInvocation) return false;
    if (_vulkan13Features.subgroupSizeControl && !features.subgroupSizeControl) return false;
    if (_vulkan13Features.computeFullSubgroups && !features.computeFullSubgroups) return false;
    if (_vulkan13Features.synchronization2 && !features.synchronization2) return false;
    if (_vulkan13Features.textureCompressionASTC_HDR && !features.textureCompressionASTC_HDR) return false;
    if (_vulkan13Features.shaderZeroInitializeWorkgroupMemory && !features.shaderZeroInitializeWorkgroupMemory) return false;
    if (_vulkan13Features.dynamicRendering && !features.dynamicRendering) return false;
    if (_vulkan13Features.shaderIntegerDotProduct && !features.shaderIntegerDotProduct) return false;
    if (_vulkan13Features.maintenance4 && !features.maintenance4) return false;
    return true;
}

bool tpd::PhysicalDeviceSelector::checkConditionalRenderingFeatures(const vk::PhysicalDeviceConditionalRenderingFeaturesEXT& features) const {
    if (_conditionalRenderingFeatures.conditionalRendering && !features.conditionalRendering) return false;
    if (_conditionalRenderingFeatures.inheritedConditionalRendering && !features.inheritedConditionalRendering) return false;
    return true;
}

bool tpd::PhysicalDeviceSelector::checkVertexInputDynamicStateFeatures(const vk::PhysicalDeviceVertexInputDynamicStateFeaturesEXT& features) const {
    if (_vertexInputDynamicStateFeatures.vertexInputDynamicState && !features.vertexInputDynamicState) return false;
    return true;
}
