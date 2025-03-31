#include "torpedo/bootstrap/PhysicalDeviceSelector.h"

#include <ranges>
#include <unordered_set>

tpd::PhysicalDeviceSelection tpd::PhysicalDeviceSelector::select(
    const vk::Instance instance,
    const std::vector<const char*>& extensions) const
{
    const auto physicalDevices = instance.enumeratePhysicalDevices();

    const auto suitable = [this, &extensions](const auto physicalDevice) {
        const auto indices = findQueueFamilies(physicalDevice);
        const auto extensionSupported = checkExtensionSupport(physicalDevice, extensions);
        const auto featureSupported   = checkPhysicalDeviceFeatures(physicalDevice);

        auto swapChainAdequate = true;
        if (_requestPresentQueueFamily && extensionSupported) {
            const auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(_surface);
            const auto formats = physicalDevice.getSurfaceFormatsKHR(_surface);
            const auto presentModes = physicalDevice.getSurfacePresentModesKHR(_surface);
            swapChainAdequate = !formats.empty() && !presentModes.empty();
        }

        return queueFamiliesComplete(indices) && extensionSupported && swapChainAdequate && featureSupported;
    };

    auto selection = PhysicalDeviceSelection{};
    auto suitableIntegratedDevice = -1;
    for (auto i = 0; i < static_cast<int>(physicalDevices.size()); ++i) {
        if (!suitable(physicalDevices[i])) {
            continue;
        }
        // Found a suitable integrated GPU, but there could be more powerful discrete GPUs waiting down the line
        if (physicalDevices[i].getProperties().deviceType != vk::PhysicalDeviceType::eDiscreteGpu) {
            suitableIntegratedDevice = i;
            continue;
        }
        // Found a discrete GPU
        selection.physicalDevice = physicalDevices[i];
        const auto [graphicsFamily, transferFamily, presentFamily, computeFamily] = findQueueFamilies(selection.physicalDevice);
        if (_requestGraphicsQueueFamily) selection.graphicsQueueFamilyIndex = graphicsFamily.value();
        /* always request transfer */    selection.transferQueueFamilyIndex = transferFamily.value();
        if (_requestPresentQueueFamily)  selection.presentQueueFamilyIndex  = presentFamily.value();
        /* always request compute */     selection.computeQueueFamilyIndex  = computeFamily.value();
        return selection;
    }

    // No suitable GPU found
    if (suitableIntegratedDevice == -1) [[unlikely]] {
        throw std::runtime_error(
            "PhysicalDeviceSelector - Failed to find a suitable device, consider requesting less extensions and features");
    }
    // Fall back to the suitable integrated GPU
    selection.physicalDevice = physicalDevices[suitableIntegratedDevice];
    const auto [graphicsFamily, transferFamily, presentFamily, computeFamily] = findQueueFamilies(selection.physicalDevice);
    if (_requestGraphicsQueueFamily) selection.graphicsQueueFamilyIndex = graphicsFamily.value();
    /* always request transfer */    selection.transferQueueFamilyIndex = transferFamily.value();
    if (_requestPresentQueueFamily)  selection.presentQueueFamilyIndex  = presentFamily.value();
    /* always request compute  */    selection.computeQueueFamilyIndex  = computeFamily.value();
    return selection;
}

bool tpd::PhysicalDeviceSelector::checkExtensionSupport(
    const vk::PhysicalDevice physicalDevice,
    const std::vector<const char*>& extensions)
{
    const auto availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();
    const auto toName = [](const vk::ExtensionProperties& extension) { return extension.extensionName.data(); };
    const auto names = availableExtensions | std::views::transform(toName) | std::ranges::to<std::unordered_set<std::string>>();

    const auto available = [&names](const auto& it) { return names.contains(it); };
    return std::ranges::all_of(extensions, std::identity{}, available);
}

tpd::PhysicalDeviceSelector::QueueFamilyIndices tpd::PhysicalDeviceSelector::findQueueFamilies(const vk::PhysicalDevice device) const {
    const auto queueFamilies = device.getQueueFamilyProperties();
    auto indices = QueueFamilyIndices{};

    // We're trying to make graphics and present the same family queue, but in case we couldn't,
    // this value would store the previously ignored present family index
    auto distinctPresentFamily = std::optional<uint32_t>{};

    auto foundAsyncTransfer = false;
    auto foundAsyncCompute  = false;

    // > From the spec: if an implementation exposes any queue family that supports graphics operations, at least one queue 
    // family of at least one physical device exposed by the implementation must support both graphics and compute operations.
    // > Also from the spec: All commands that are allowed on a queue that supports transfer operations are also allowed 
    // on a queue that supports either graphics or compute operations thus if the capabilities of a queue family 
    // include VK_QUEUE_GRAPHICS_BIT or VK_QUEUE_COMPUTE_BIT then reporting the VK_QUEUE_TRANSFER_BIT capability 
    // separately for that queue family is optional.

    for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
        // This may or may not be a distinct transfer family, we overwrite any previously set value
        // to favor async transfer, even if the call site didn't explicitly request for one
        if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eTransfer &&
            !(queueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute)) {
            indices.transferFamily = i;
            foundAsyncTransfer = true;
        }

        if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            if (!indices.graphicsFamily.has_value() && _requestGraphicsQueueFamily) {
                indices.graphicsFamily = i;  // set graphics if being requested for
            }
            // We always set transfer to graphics first, making sure transfer is at least distinct from compute.
            // The check further favors async transfer even if the call site didn't explicitly request.
            if (!indices.transferFamily.has_value()) {
                indices.transferFamily = i;
            }
        }

        if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute) {
            indices.computeFamily = i;
            if (!(queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics)) {
                foundAsyncCompute = true;
            }
            if (!indices.transferFamily.has_value()) {
                indices.transferFamily = i;  // this GPU may support no graphics operation
            }
        }

        if (!indices.presentFamily.has_value() && _requestPresentQueueFamily && device.getSurfaceSupportKHR(i, _surface)) {
            indices.presentFamily = i;
            // In some (rare) cases, if we've found a present family whose index is different from that of graphics,
            // we ignore it but remember its index in case we cannot find a family supports both graphics and present
            if (_requestGraphicsQueueFamily && !indices.graphicsFamily.has_value() ||
                indices.graphicsFamily.has_value() && i != indices.graphicsFamily.value()) [[unlikely]] {
                distinctPresentFamily = i;
                indices.presentFamily.reset();
            }
        }

        // Discard the wrong index for compute family if async compute is being requested
        if (_requestGraphicsQueueFamily && _asyncCompute &&
            indices.graphicsFamily.has_value() && indices.computeFamily.has_value() &&
            indices.graphicsFamily.value() == indices.computeFamily.value()) {
            indices.computeFamily.reset();
        }

        // Discard the wrong index for transfer family if async transfer is being requested
        // We favor async transfer to be distinct from compute
        if (_requestGraphicsQueueFamily && _asyncTransfer &&
            indices.graphicsFamily.has_value() && indices.transferFamily.has_value() &&
            indices.graphicsFamily.value() == indices.transferFamily.value()) {
            indices.transferFamily.reset();
        }

        if (queueFamiliesComplete(indices) && foundAsyncTransfer && foundAsyncCompute) {
            break;
        }
    }

    // Fall back to the found separate present if there was no graphics family supporting present
    if (!indices.presentFamily.has_value() && distinctPresentFamily.has_value()) {
        indices.presentFamily = distinctPresentFamily.value();
    }

    return indices;
}

bool tpd::PhysicalDeviceSelector::queueFamiliesComplete(const QueueFamilyIndices& indices) const {
    if (_requestGraphicsQueueFamily && !indices.graphicsFamily.has_value()) return false;
    if (/* always request transfer */  !indices.transferFamily.has_value()) return false;
    if (_requestPresentQueueFamily  && !indices.presentFamily.has_value())  return false;
    if (/* always request compute  */  !indices.computeFamily.has_value())  return false;
    if (_asyncCompute  && indices.graphicsFamily == indices.computeFamily)  return false;
    if (_asyncTransfer && indices.graphicsFamily == indices.transferFamily) return false;
    return true;
}

bool tpd::PhysicalDeviceSelector::checkPhysicalDeviceFeatures(const vk::PhysicalDevice physicalDevice) const {
    auto features = vk::PhysicalDeviceFeatures{};
    physicalDevice.getFeatures(&features);

    auto extendedDynamicState1Features = vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT{};
    auto extendedDynamicState2Features = vk::PhysicalDeviceExtendedDynamicState2FeaturesEXT{};
    auto extendedDynamicState3Features = vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT{};
    auto vulkan11Features = vk::PhysicalDeviceVulkan11Features{};
    auto vulkan12Features = vk::PhysicalDeviceVulkan12Features{};
    auto vulkan13Features = vk::PhysicalDeviceVulkan13Features{};
    auto vulkan14Features = vk::PhysicalDeviceVulkan14Features{};
    auto descriptorIndexingFeatures = vk::PhysicalDeviceDescriptorIndexingFeatures{};
    auto dynamicRenderingFeatures = vk::PhysicalDeviceDynamicRenderingFeatures{};
    auto synchronization2Features = vk::PhysicalDeviceSynchronization2Features{};
    auto timelineSemaphoreFeatures = vk::PhysicalDeviceTimelineSemaphoreFeatures{};
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
    vulkan13Features.pNext = &vulkan14Features;
    vulkan14Features.pNext = &descriptorIndexingFeatures;
    descriptorIndexingFeatures.pNext = &dynamicRenderingFeatures;
    dynamicRenderingFeatures.pNext = &synchronization2Features;
    synchronization2Features.pNext = &timelineSemaphoreFeatures;
    timelineSemaphoreFeatures.pNext = &conditionalRenderingFeatures;
    conditionalRenderingFeatures.pNext = &vertexInputDynamicStateFeatures;

    physicalDevice.getFeatures2(&supportedFeatures);

    return checkFeatures(features) &&
        checkExtendedDynamicState1Features(extendedDynamicState1Features) &&
        checkExtendedDynamicState2Features(extendedDynamicState2Features) &&
        checkExtendedDynamicState3Features(extendedDynamicState3Features) &&
        checkVulkan11Features(vulkan11Features) &&
        checkVulkan12Features(vulkan12Features) &&
        checkVulkan13Features(vulkan13Features) &&
        checkVulkan14Features(vulkan14Features) &&
        checkDescriptorIndexingFeatures(descriptorIndexingFeatures) &&
        checkDynamicRenderingFeatures(dynamicRenderingFeatures) &&
        checkSynchronization2Features(synchronization2Features) &&
        checkTimelineSemaphoreFeatures(timelineSemaphoreFeatures) &&
        checkConditionalRenderingFeatures(conditionalRenderingFeatures) &&
        checkVertexInputDynamicStateFeatures(vertexInputDynamicStateFeatures);
}

bool tpd::PhysicalDeviceSelector::checkFeatures(const vk::PhysicalDeviceFeatures& features) const {
    if (_features.robustBufferAccess && !features.robustBufferAccess) [[unlikely]] return false;
    if (_features.fullDrawIndexUint32 && !features.fullDrawIndexUint32) [[unlikely]] return false;
    if (_features.imageCubeArray && !features.imageCubeArray) [[unlikely]] return false;
    if (_features.independentBlend && !features.independentBlend) [[unlikely]] return false;
    if (_features.geometryShader && !features.geometryShader) [[unlikely]] return false;
    if (_features.tessellationShader && !features.tessellationShader) [[unlikely]] return false;
    if (_features.sampleRateShading && !features.sampleRateShading) [[unlikely]] return false;
    if (_features.dualSrcBlend && !features.dualSrcBlend) [[unlikely]] return false;
    if (_features.logicOp && !features.logicOp) [[unlikely]] return false;
    if (_features.multiDrawIndirect && !features.multiDrawIndirect) [[unlikely]] return false;
    if (_features.drawIndirectFirstInstance && !features.drawIndirectFirstInstance) [[unlikely]] return false;
    if (_features.depthClamp && !features.depthClamp) [[unlikely]] return false;
    if (_features.depthBiasClamp && !features.depthBiasClamp) [[unlikely]] return false;
    if (_features.fillModeNonSolid && !features.fillModeNonSolid) [[unlikely]] return false;
    if (_features.depthBounds && !features.depthBounds) [[unlikely]] return false;
    if (_features.wideLines && !features.wideLines) [[unlikely]] return false;
    if (_features.largePoints && !features.largePoints) [[unlikely]] return false;
    if (_features.alphaToOne && !features.alphaToOne) [[unlikely]] return false;
    if (_features.multiViewport && !features.multiViewport) [[unlikely]] return false;
    if (_features.samplerAnisotropy && !features.samplerAnisotropy) [[unlikely]] return false;
    if (_features.textureCompressionETC2 && !features.textureCompressionETC2) [[unlikely]] return false;
    if (_features.textureCompressionASTC_LDR && !features.textureCompressionASTC_LDR) [[unlikely]] return false;
    if (_features.textureCompressionBC && !features.textureCompressionBC) [[unlikely]] return false;
    if (_features.occlusionQueryPrecise && !features.occlusionQueryPrecise) [[unlikely]] return false;
    if (_features.pipelineStatisticsQuery && !features.pipelineStatisticsQuery) [[unlikely]] return false;
    if (_features.vertexPipelineStoresAndAtomics && !features.vertexPipelineStoresAndAtomics) [[unlikely]] return false;
    if (_features.fragmentStoresAndAtomics && !features.fragmentStoresAndAtomics) [[unlikely]] return false;
    if (_features.shaderTessellationAndGeometryPointSize && !features.shaderTessellationAndGeometryPointSize) [[unlikely]] return false;
    if (_features.shaderImageGatherExtended && !features.shaderImageGatherExtended) [[unlikely]] return false;
    if (_features.shaderStorageImageExtendedFormats && !features.shaderStorageImageExtendedFormats) [[unlikely]] return false;
    if (_features.shaderStorageImageMultisample && !features.shaderStorageImageMultisample) [[unlikely]] return false;
    if (_features.shaderStorageImageReadWithoutFormat && !features.shaderStorageImageReadWithoutFormat) [[unlikely]] return false;
    if (_features.shaderStorageImageWriteWithoutFormat && !features.shaderStorageImageWriteWithoutFormat) [[unlikely]] return false;
    if (_features.shaderUniformBufferArrayDynamicIndexing && !features.shaderUniformBufferArrayDynamicIndexing) [[unlikely]] return false;
    if (_features.shaderSampledImageArrayDynamicIndexing && !features.shaderSampledImageArrayDynamicIndexing) [[unlikely]] return false;
    if (_features.shaderStorageBufferArrayDynamicIndexing && !features.shaderStorageBufferArrayDynamicIndexing) [[unlikely]] return false;
    if (_features.shaderStorageImageArrayDynamicIndexing && !features.shaderStorageImageArrayDynamicIndexing) [[unlikely]] return false;
    if (_features.shaderClipDistance && !features.shaderClipDistance) [[unlikely]] return false;
    if (_features.shaderCullDistance && !features.shaderCullDistance) [[unlikely]] return false;
    if (_features.shaderFloat64 && !features.shaderFloat64) [[unlikely]] return false;
    if (_features.shaderInt64 && !features.shaderInt64) [[unlikely]] return false;
    if (_features.shaderInt16 && !features.shaderInt16) [[unlikely]] return false;
    if (_features.shaderResourceResidency && !features.shaderResourceResidency) [[unlikely]] return false;
    if (_features.shaderResourceMinLod && !features.shaderResourceMinLod) [[unlikely]] return false;
    if (_features.sparseBinding && !features.sparseBinding) [[unlikely]] return false;
    if (_features.sparseResidencyBuffer && !features.sparseResidencyBuffer) [[unlikely]] return false;
    if (_features.sparseResidencyImage2D && !features.sparseResidencyImage2D) [[unlikely]] return false;
    if (_features.sparseResidencyImage3D && !features.sparseResidencyImage3D) [[unlikely]] return false;
    if (_features.sparseResidency2Samples && !features.sparseResidency2Samples) [[unlikely]] return false;
    if (_features.sparseResidency4Samples && !features.sparseResidency4Samples) [[unlikely]] return false;
    if (_features.sparseResidency8Samples && !features.sparseResidency8Samples) [[unlikely]] return false;
    if (_features.sparseResidency16Samples && !features.sparseResidency16Samples) [[unlikely]] return false;
    if (_features.sparseResidencyAliased && !features.sparseResidencyAliased) [[unlikely]] return false;
    if (_features.variableMultisampleRate && !features.variableMultisampleRate) [[unlikely]] return false;
    if (_features.inheritedQueries && !features.inheritedQueries) [[unlikely]] return false;
    return true;
}

bool tpd::PhysicalDeviceSelector::checkExtendedDynamicState1Features(const vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT& features) const {
    if (_extendedDynamicState1Features.extendedDynamicState && !features.extendedDynamicState) [[unlikely]] return false;
    return true;
}

bool tpd::PhysicalDeviceSelector::checkExtendedDynamicState2Features(const vk::PhysicalDeviceExtendedDynamicState2FeaturesEXT& features) const {
    if (_extendedDynamicState2Features.extendedDynamicState2 && !features.extendedDynamicState2) [[unlikely]] return false;
    if (_extendedDynamicState2Features.extendedDynamicState2LogicOp && !features.extendedDynamicState2LogicOp) [[unlikely]] return false;
    if (_extendedDynamicState2Features.extendedDynamicState2PatchControlPoints && !features.extendedDynamicState2PatchControlPoints) [[unlikely]] return false;
    return true;
}

bool tpd::PhysicalDeviceSelector::checkExtendedDynamicState3Features(const vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT& features) const {
    if (_extendedDynamicState3Features.extendedDynamicState3TessellationDomainOrigin && !features.extendedDynamicState3TessellationDomainOrigin) [[unlikely]] return false;
    if (_extendedDynamicState3Features.extendedDynamicState3DepthClampEnable && !features.extendedDynamicState3DepthClampEnable) [[unlikely]] return false;
    if (_extendedDynamicState3Features.extendedDynamicState3PolygonMode && !features.extendedDynamicState3PolygonMode) [[unlikely]] return false;
    if (_extendedDynamicState3Features.extendedDynamicState3RasterizationSamples && !features.extendedDynamicState3RasterizationSamples) [[unlikely]] return false;
    if (_extendedDynamicState3Features.extendedDynamicState3SampleMask && !features.extendedDynamicState3SampleMask) [[unlikely]] return false;
    if (_extendedDynamicState3Features.extendedDynamicState3AlphaToCoverageEnable && !features.extendedDynamicState3AlphaToCoverageEnable) [[unlikely]] return false;
    if (_extendedDynamicState3Features.extendedDynamicState3AlphaToOneEnable && !features.extendedDynamicState3AlphaToOneEnable) [[unlikely]] return false;
    if (_extendedDynamicState3Features.extendedDynamicState3LogicOpEnable && !features.extendedDynamicState3LogicOpEnable) [[unlikely]] return false;
    if (_extendedDynamicState3Features.extendedDynamicState3ColorBlendEnable && !features.extendedDynamicState3ColorBlendEnable) [[unlikely]] return false;
    if (_extendedDynamicState3Features.extendedDynamicState3ColorBlendEquation && !features.extendedDynamicState3ColorBlendEquation) [[unlikely]] return false;
    if (_extendedDynamicState3Features.extendedDynamicState3ColorWriteMask && !features.extendedDynamicState3ColorWriteMask) [[unlikely]] return false;
    if (_extendedDynamicState3Features.extendedDynamicState3RasterizationStream && !features.extendedDynamicState3RasterizationStream) [[unlikely]] return false;
    if (_extendedDynamicState3Features.extendedDynamicState3ConservativeRasterizationMode && !features.extendedDynamicState3ConservativeRasterizationMode) [[unlikely]] return false;
    if (_extendedDynamicState3Features.extendedDynamicState3ExtraPrimitiveOverestimationSize && !features.extendedDynamicState3ExtraPrimitiveOverestimationSize) [[unlikely]] return false;
    if (_extendedDynamicState3Features.extendedDynamicState3DepthClipEnable && !features.extendedDynamicState3DepthClipEnable) [[unlikely]] return false;
    if (_extendedDynamicState3Features.extendedDynamicState3SampleLocationsEnable && !features.extendedDynamicState3SampleLocationsEnable) [[unlikely]] return false;
    if (_extendedDynamicState3Features.extendedDynamicState3ColorBlendAdvanced && !features.extendedDynamicState3ColorBlendAdvanced) [[unlikely]] return false;
    if (_extendedDynamicState3Features.extendedDynamicState3ProvokingVertexMode && !features.extendedDynamicState3ProvokingVertexMode) [[unlikely]] return false;
    if (_extendedDynamicState3Features.extendedDynamicState3LineRasterizationMode && !features.extendedDynamicState3LineRasterizationMode) [[unlikely]] return false;
    if (_extendedDynamicState3Features.extendedDynamicState3LineStippleEnable && !features.extendedDynamicState3LineStippleEnable) [[unlikely]] return false;
    if (_extendedDynamicState3Features.extendedDynamicState3DepthClipNegativeOneToOne && !features.extendedDynamicState3DepthClipNegativeOneToOne) [[unlikely]] return false;
    if (_extendedDynamicState3Features.extendedDynamicState3ViewportWScalingEnable && !features.extendedDynamicState3ViewportWScalingEnable) [[unlikely]] return false;
    if (_extendedDynamicState3Features.extendedDynamicState3ViewportSwizzle && !features.extendedDynamicState3ViewportSwizzle) [[unlikely]] return false;
    if (_extendedDynamicState3Features.extendedDynamicState3CoverageToColorEnable && !features.extendedDynamicState3CoverageToColorEnable) [[unlikely]] return false;
    if (_extendedDynamicState3Features.extendedDynamicState3CoverageToColorLocation && !features.extendedDynamicState3CoverageToColorLocation) [[unlikely]] return false;
    if (_extendedDynamicState3Features.extendedDynamicState3CoverageModulationMode && !features.extendedDynamicState3CoverageModulationMode) [[unlikely]] return false;
    if (_extendedDynamicState3Features.extendedDynamicState3CoverageModulationTableEnable && !features.extendedDynamicState3CoverageModulationTableEnable) [[unlikely]] return false;
    if (_extendedDynamicState3Features.extendedDynamicState3CoverageModulationTable && !features.extendedDynamicState3CoverageModulationTable) [[unlikely]] return false;
    if (_extendedDynamicState3Features.extendedDynamicState3CoverageReductionMode && !features.extendedDynamicState3CoverageReductionMode) [[unlikely]] return false;
    if (_extendedDynamicState3Features.extendedDynamicState3RepresentativeFragmentTestEnable && !features.extendedDynamicState3RepresentativeFragmentTestEnable) [[unlikely]] return false;
    if (_extendedDynamicState3Features.extendedDynamicState3ShadingRateImageEnable && !features.extendedDynamicState3ShadingRateImageEnable) [[unlikely]] return false;
    return true;
}

bool tpd::PhysicalDeviceSelector::checkVulkan11Features(const vk::PhysicalDeviceVulkan11Features& features) const {
    if (_vulkan11Features.storageBuffer16BitAccess && !features.storageBuffer16BitAccess) [[unlikely]] return false;
    if (_vulkan11Features.uniformAndStorageBuffer16BitAccess && !features.uniformAndStorageBuffer16BitAccess) [[unlikely]] return false;
    if (_vulkan11Features.storagePushConstant16 && !features.storagePushConstant16) [[unlikely]] return false;
    if (_vulkan11Features.storageInputOutput16 && !features.storageInputOutput16) [[unlikely]] return false;
    if (_vulkan11Features.multiview && !features.multiview) [[unlikely]] return false;
    if (_vulkan11Features.multiviewGeometryShader && !features.multiviewGeometryShader) [[unlikely]] return false;
    if (_vulkan11Features.multiviewTessellationShader && !features.multiviewTessellationShader) [[unlikely]] return false;
    if (_vulkan11Features.variablePointersStorageBuffer && !features.variablePointersStorageBuffer) [[unlikely]] return false;
    if (_vulkan11Features.variablePointers && !features.variablePointers) [[unlikely]] return false;
    if (_vulkan11Features.protectedMemory && !features.protectedMemory) [[unlikely]] return false;
    if (_vulkan11Features.samplerYcbcrConversion && !features.samplerYcbcrConversion) [[unlikely]] return false;
    if (_vulkan11Features.shaderDrawParameters && !features.shaderDrawParameters) [[unlikely]] return false;
    return true;
}

bool tpd::PhysicalDeviceSelector::checkVulkan12Features(const vk::PhysicalDeviceVulkan12Features& features) const {
    if (_vulkan12Features.samplerMirrorClampToEdge && !features.samplerMirrorClampToEdge) [[unlikely]] return false;
    if (_vulkan12Features.drawIndirectCount && !features.drawIndirectCount) [[unlikely]] return false;
    if (_vulkan12Features.storageBuffer8BitAccess && !features.storageBuffer8BitAccess) [[unlikely]] return false;
    if (_vulkan12Features.uniformAndStorageBuffer8BitAccess && !features.uniformAndStorageBuffer8BitAccess) [[unlikely]] return false;
    if (_vulkan12Features.storagePushConstant8 && !features.storagePushConstant8) [[unlikely]] return false;
    if (_vulkan12Features.shaderBufferInt64Atomics && !features.shaderBufferInt64Atomics) [[unlikely]] return false;
    if (_vulkan12Features.shaderSharedInt64Atomics && !features.shaderSharedInt64Atomics) [[unlikely]] return false;
    if (_vulkan12Features.shaderFloat16 && !features.shaderFloat16) [[unlikely]] return false;
    if (_vulkan12Features.shaderInt8 && !features.shaderInt8) [[unlikely]] return false;
    if (_vulkan12Features.descriptorIndexing && !features.descriptorIndexing) [[unlikely]] return false;
    if (_vulkan12Features.shaderInputAttachmentArrayDynamicIndexing && !features.shaderInputAttachmentArrayDynamicIndexing) [[unlikely]] return false;
    if (_vulkan12Features.shaderUniformTexelBufferArrayDynamicIndexing && !features.shaderUniformTexelBufferArrayDynamicIndexing) [[unlikely]] return false;
    if (_vulkan12Features.shaderStorageTexelBufferArrayDynamicIndexing && !features.shaderStorageTexelBufferArrayDynamicIndexing) [[unlikely]] return false;
    if (_vulkan12Features.shaderUniformBufferArrayNonUniformIndexing && !features.shaderUniformBufferArrayNonUniformIndexing) [[unlikely]] return false;
    if (_vulkan12Features.shaderSampledImageArrayNonUniformIndexing && !features.shaderSampledImageArrayNonUniformIndexing) [[unlikely]] return false;
    if (_vulkan12Features.shaderStorageBufferArrayNonUniformIndexing && !features.shaderStorageBufferArrayNonUniformIndexing) [[unlikely]] return false;
    if (_vulkan12Features.shaderStorageImageArrayNonUniformIndexing && !features.shaderStorageImageArrayNonUniformIndexing) [[unlikely]] return false;
    if (_vulkan12Features.shaderInputAttachmentArrayNonUniformIndexing && !features.shaderInputAttachmentArrayNonUniformIndexing) [[unlikely]] return false;
    if (_vulkan12Features.shaderUniformTexelBufferArrayNonUniformIndexing && !features.shaderUniformTexelBufferArrayNonUniformIndexing) [[unlikely]] return false;
    if (_vulkan12Features.shaderStorageTexelBufferArrayNonUniformIndexing && !features.shaderStorageTexelBufferArrayNonUniformIndexing) [[unlikely]] return false;
    if (_vulkan12Features.descriptorBindingUniformBufferUpdateAfterBind && !features.descriptorBindingUniformBufferUpdateAfterBind) [[unlikely]] return false;
    if (_vulkan12Features.descriptorBindingSampledImageUpdateAfterBind && !features.descriptorBindingSampledImageUpdateAfterBind) [[unlikely]] return false;
    if (_vulkan12Features.descriptorBindingStorageImageUpdateAfterBind && !features.descriptorBindingStorageImageUpdateAfterBind) [[unlikely]] return false;
    if (_vulkan12Features.descriptorBindingStorageBufferUpdateAfterBind && !features.descriptorBindingStorageBufferUpdateAfterBind) [[unlikely]] return false;
    if (_vulkan12Features.descriptorBindingUniformTexelBufferUpdateAfterBind && !features.descriptorBindingUniformTexelBufferUpdateAfterBind) [[unlikely]] return false;
    if (_vulkan12Features.descriptorBindingStorageTexelBufferUpdateAfterBind && !features.descriptorBindingStorageTexelBufferUpdateAfterBind) [[unlikely]] return false;
    if (_vulkan12Features.descriptorBindingUpdateUnusedWhilePending && !features.descriptorBindingUpdateUnusedWhilePending) [[unlikely]] return false;
    if (_vulkan12Features.descriptorBindingPartiallyBound && !features.descriptorBindingPartiallyBound) [[unlikely]] return false;
    if (_vulkan12Features.descriptorBindingVariableDescriptorCount && !features.descriptorBindingVariableDescriptorCount) [[unlikely]] return false;
    if (_vulkan12Features.runtimeDescriptorArray && !features.runtimeDescriptorArray) [[unlikely]] return false;
    if (_vulkan12Features.samplerFilterMinmax && !features.samplerFilterMinmax) [[unlikely]] return false;
    if (_vulkan12Features.scalarBlockLayout && !features.scalarBlockLayout) [[unlikely]] return false;
    if (_vulkan12Features.imagelessFramebuffer && !features.imagelessFramebuffer) [[unlikely]] return false;
    if (_vulkan12Features.uniformBufferStandardLayout && !features.uniformBufferStandardLayout) [[unlikely]] return false;
    if (_vulkan12Features.shaderSubgroupExtendedTypes && !features.shaderSubgroupExtendedTypes) [[unlikely]] return false;
    if (_vulkan12Features.separateDepthStencilLayouts && !features.separateDepthStencilLayouts) [[unlikely]] return false;
    if (_vulkan12Features.hostQueryReset && !features.hostQueryReset) [[unlikely]] return false;
    if (_vulkan12Features.timelineSemaphore && !features.timelineSemaphore) [[unlikely]] return false;
    if (_vulkan12Features.bufferDeviceAddress && !features.bufferDeviceAddress) [[unlikely]] return false;
    if (_vulkan12Features.bufferDeviceAddressCaptureReplay && !features.bufferDeviceAddressCaptureReplay) [[unlikely]] return false;
    if (_vulkan12Features.bufferDeviceAddressMultiDevice && !features.bufferDeviceAddressMultiDevice) [[unlikely]] return false;
    if (_vulkan12Features.vulkanMemoryModel && !features.vulkanMemoryModel) [[unlikely]] return false;
    if (_vulkan12Features.vulkanMemoryModelDeviceScope && !features.vulkanMemoryModelDeviceScope) [[unlikely]] return false;
    if (_vulkan12Features.vulkanMemoryModelAvailabilityVisibilityChains && !features.vulkanMemoryModelAvailabilityVisibilityChains) [[unlikely]] return false;
    if (_vulkan12Features.shaderOutputViewportIndex && !features.shaderOutputViewportIndex) [[unlikely]] return false;
    if (_vulkan12Features.shaderOutputLayer && !features.shaderOutputLayer) [[unlikely]] return false;
    if (_vulkan12Features.subgroupBroadcastDynamicId && !features.subgroupBroadcastDynamicId) [[unlikely]] return false;
    return true;
}

bool tpd::PhysicalDeviceSelector::checkVulkan13Features(const vk::PhysicalDeviceVulkan13Features& features) const {
    if (_vulkan13Features.robustImageAccess && !features.robustImageAccess) [[unlikely]] return false;
    if (_vulkan13Features.inlineUniformBlock && !features.inlineUniformBlock) [[unlikely]] return false;
    if (_vulkan13Features.descriptorBindingInlineUniformBlockUpdateAfterBind && !features.descriptorBindingInlineUniformBlockUpdateAfterBind) [[unlikely]] return false;
    if (_vulkan13Features.pipelineCreationCacheControl && !features.pipelineCreationCacheControl) [[unlikely]] return false;
    if (_vulkan13Features.privateData && !features.privateData) [[unlikely]] return false;
    if (_vulkan13Features.shaderDemoteToHelperInvocation && !features.shaderDemoteToHelperInvocation) [[unlikely]] return false;
    if (_vulkan13Features.shaderTerminateInvocation && !features.shaderTerminateInvocation) [[unlikely]] return false;
    if (_vulkan13Features.subgroupSizeControl && !features.subgroupSizeControl) [[unlikely]] return false;
    if (_vulkan13Features.computeFullSubgroups && !features.computeFullSubgroups) [[unlikely]] return false;
    if (_vulkan13Features.synchronization2 && !features.synchronization2) [[unlikely]] return false;
    if (_vulkan13Features.textureCompressionASTC_HDR && !features.textureCompressionASTC_HDR) [[unlikely]] return false;
    if (_vulkan13Features.shaderZeroInitializeWorkgroupMemory && !features.shaderZeroInitializeWorkgroupMemory) [[unlikely]] return false;
    if (_vulkan13Features.dynamicRendering && !features.dynamicRendering) [[unlikely]] return false;
    if (_vulkan13Features.shaderIntegerDotProduct && !features.shaderIntegerDotProduct) [[unlikely]] return false;
    if (_vulkan13Features.maintenance4 && !features.maintenance4) [[unlikely]] return false;
    return true;
}

bool tpd::PhysicalDeviceSelector::checkVulkan14Features(const vk::PhysicalDeviceVulkan14Features& features) const {
    if (_vulkan14Features.globalPriorityQuery && !features.globalPriorityQuery) [[unlikely]] return false;
    if (_vulkan14Features.shaderSubgroupRotate && !features.shaderSubgroupRotate) [[unlikely]] return false;
    if (_vulkan14Features.shaderSubgroupRotateClustered && !features.shaderSubgroupRotateClustered) [[unlikely]] return false;
    if (_vulkan14Features.shaderFloatControls2 && !features.shaderFloatControls2) [[unlikely]] return false;
    if (_vulkan14Features.shaderExpectAssume && !features.shaderExpectAssume) [[unlikely]] return false;
    if (_vulkan14Features.rectangularLines && !features.rectangularLines) [[unlikely]] return false;
    if (_vulkan14Features.bresenhamLines && !features.bresenhamLines) [[unlikely]] return false;
    if (_vulkan14Features.smoothLines && !features.smoothLines) [[unlikely]] return false;
    if (_vulkan14Features.stippledRectangularLines && !features.stippledRectangularLines) [[unlikely]] return false;
    if (_vulkan14Features.stippledBresenhamLines && !features.stippledBresenhamLines) [[unlikely]] return false;
    if (_vulkan14Features.stippledSmoothLines && !features.stippledSmoothLines) [[unlikely]] return false;
    if (_vulkan14Features.vertexAttributeInstanceRateDivisor && !features.vertexAttributeInstanceRateDivisor) [[unlikely]] return false;
    if (_vulkan14Features.vertexAttributeInstanceRateZeroDivisor && !features.vertexAttributeInstanceRateZeroDivisor) [[unlikely]] return false;
    if (_vulkan14Features.indexTypeUint8 && !features.indexTypeUint8) [[unlikely]] return false;
    if (_vulkan14Features.dynamicRenderingLocalRead && !features.dynamicRenderingLocalRead) [[unlikely]] return false;
    if (_vulkan14Features.maintenance5 && !features.maintenance5) [[unlikely]] return false;
    if (_vulkan14Features.maintenance6 && !features.maintenance6) [[unlikely]] return false;
    if (_vulkan14Features.pipelineProtectedAccess && !features.pipelineProtectedAccess) [[unlikely]] return false;
    if (_vulkan14Features.pipelineRobustness && !features.pipelineRobustness) [[unlikely]] return false;
    if (_vulkan14Features.hostImageCopy && !features.hostImageCopy) [[unlikely]] return false;
    if (_vulkan14Features.pushDescriptor && !features.pushDescriptor) [[unlikely]] return false;
    return true;
}

bool tpd::PhysicalDeviceSelector::checkDescriptorIndexingFeatures(const vk::PhysicalDeviceDescriptorIndexingFeaturesEXT& features) const {
    if (_descriptorIndexingFeatures.descriptorBindingPartiallyBound && !features.descriptorBindingPartiallyBound) [[unlikely]] return false;
    if (_descriptorIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind && !features.descriptorBindingSampledImageUpdateAfterBind) [[unlikely]] return false;
    if (_descriptorIndexingFeatures.descriptorBindingStorageBufferUpdateAfterBind && !features.descriptorBindingStorageBufferUpdateAfterBind) [[unlikely]] return false;
    if (_descriptorIndexingFeatures.descriptorBindingStorageImageUpdateAfterBind && !features.descriptorBindingStorageImageUpdateAfterBind) [[unlikely]] return false;
    if (_descriptorIndexingFeatures.descriptorBindingStorageTexelBufferUpdateAfterBind && !features.descriptorBindingStorageTexelBufferUpdateAfterBind) [[unlikely]] return false;
    if (_descriptorIndexingFeatures.descriptorBindingUniformBufferUpdateAfterBind && !features.descriptorBindingUniformBufferUpdateAfterBind) [[unlikely]] return false;
    if (_descriptorIndexingFeatures.descriptorBindingUniformTexelBufferUpdateAfterBind && !features.descriptorBindingUniformTexelBufferUpdateAfterBind) [[unlikely]] return false;
    if (_descriptorIndexingFeatures.descriptorBindingUpdateUnusedWhilePending && !features.descriptorBindingUpdateUnusedWhilePending) [[unlikely]] return false;
    if (_descriptorIndexingFeatures.descriptorBindingVariableDescriptorCount && !features.descriptorBindingVariableDescriptorCount) [[unlikely]] return false;
    if (_descriptorIndexingFeatures.shaderInputAttachmentArrayDynamicIndexing && !features.shaderInputAttachmentArrayDynamicIndexing) [[unlikely]] return false;
    if (_descriptorIndexingFeatures.shaderInputAttachmentArrayNonUniformIndexing && !features.shaderInputAttachmentArrayNonUniformIndexing) [[unlikely]] return false;
    if (_descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing && !features.shaderSampledImageArrayNonUniformIndexing) [[unlikely]] return false;
    if (_descriptorIndexingFeatures.shaderStorageBufferArrayNonUniformIndexing && !features.shaderStorageBufferArrayNonUniformIndexing) [[unlikely]] return false;
    if (_descriptorIndexingFeatures.shaderStorageImageArrayNonUniformIndexing && !features.shaderStorageImageArrayNonUniformIndexing) [[unlikely]] return false;
    if (_descriptorIndexingFeatures.shaderUniformBufferArrayNonUniformIndexing && !features.shaderUniformBufferArrayNonUniformIndexing) [[unlikely]] return false;
    if (_descriptorIndexingFeatures.shaderStorageTexelBufferArrayDynamicIndexing && !features.shaderStorageTexelBufferArrayDynamicIndexing) [[unlikely]] return false;
    if (_descriptorIndexingFeatures.shaderStorageTexelBufferArrayNonUniformIndexing && !features.shaderStorageTexelBufferArrayNonUniformIndexing) [[unlikely]] return false;
    if (_descriptorIndexingFeatures.shaderUniformTexelBufferArrayDynamicIndexing && !features.shaderUniformTexelBufferArrayDynamicIndexing) [[unlikely]] return false;
    if (_descriptorIndexingFeatures.shaderUniformTexelBufferArrayNonUniformIndexing && !features.shaderUniformTexelBufferArrayNonUniformIndexing) [[unlikely]] return false;
    if (_descriptorIndexingFeatures.runtimeDescriptorArray && !features.runtimeDescriptorArray) [[unlikely]] return false;
    return true;
}

bool tpd::PhysicalDeviceSelector::checkDynamicRenderingFeatures(const vk::PhysicalDeviceDynamicRenderingFeatures& features) const {
    if (_dynamicRenderingFeatures.dynamicRendering && !features.dynamicRendering) [[unlikely]] return false;
    return true;
}

bool tpd::PhysicalDeviceSelector::checkSynchronization2Features(const vk::PhysicalDeviceSynchronization2Features& features) const {
    if (_synchronization2Features.synchronization2 && !features.synchronization2) [[unlikely]] return false;
    return true;
}

bool tpd::PhysicalDeviceSelector::checkTimelineSemaphoreFeatures(const vk::PhysicalDeviceTimelineSemaphoreFeatures& features) const {
    if (_timelineSemaphoreFeatures.timelineSemaphore && !features.timelineSemaphore) [[unlikely]] return false;
    return true;
}

bool tpd::PhysicalDeviceSelector::checkConditionalRenderingFeatures(const vk::PhysicalDeviceConditionalRenderingFeaturesEXT& features) const {
    if (_conditionalRenderingFeatures.conditionalRendering && !features.conditionalRendering) [[unlikely]] return false;
    if (_conditionalRenderingFeatures.inheritedConditionalRendering && !features.inheritedConditionalRendering) [[unlikely]] return false;
    return true;
}

bool tpd::PhysicalDeviceSelector::checkVertexInputDynamicStateFeatures(const vk::PhysicalDeviceVertexInputDynamicStateFeaturesEXT& features) const {
    if (_vertexInputDynamicStateFeatures.vertexInputDynamicState && !features.vertexInputDynamicState) [[unlikely]] return false;
    return true;
}
