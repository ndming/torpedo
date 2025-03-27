#include "torpedo/volumetric/GaussianEngine.h"

#include <torpedo/bootstrap/DeviceBuilder.h>
#include <torpedo/bootstrap/ShaderModuleBuilder.h>
#include <torpedo/foundation/ImageUtils.h>

#include <filesystem>

void tpd::GaussianEngine::preFramePass() {
    // Pre-frame pass only applies to async compute
    if (_graphicsFamilyIndex == _computeFamilyIndex) {
        return;
    }
    const auto currentFrame = _renderer->getCurrentDrawingFrame();

    using limits = std::numeric_limits<uint64_t>;
    [[maybe_unused]] const auto result = _device.waitForFences(_computeSyncs[currentFrame].computeDrawFence, vk::True, limits::max());
    
    // This is the place to update UBOs (cameras, etc. )
    
    _device.resetFences(_computeSyncs[currentFrame].computeDrawFence);
    const auto computeDraw = _computeCommandBuffers[currentFrame];

    computeDraw.reset();
    computeDraw.begin(vk::CommandBufferBeginInfo{});

    // Don't perform ownership transfer from graphics -> compute here since we don't care about the old content
    // Undefined contents will be cleared by the compute shader
    _targets[currentFrame].recordImageTransition(computeDraw, vk::ImageLayout::eGeneral);

    computeDraw.bindPipeline(vk::PipelineBindPoint::eCompute, _pipeline);
    computeDraw.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute, _shaderLayout->getPipelineLayout(),
        /* first set */ 0, _shaderInstance->getDescriptorSets(currentFrame), {});
    
    const auto [w, h, _] = _targets[currentFrame].getPixelSize();
    computeDraw.dispatch(std::ceil(w / 16.0f), std::ceil(h / 16.0f), 1);

    // Transfer ownership to graphics before submitting to the compute queue
    _targets[currentFrame].recordOwnershipRelease(computeDraw, _computeFamilyIndex, _graphicsFamilyIndex);
    computeDraw.end();

    const auto computeInfo = vk::CommandBufferSubmitInfo{}
        .setCommandBuffer(computeDraw)
        .setDeviceMask(0b1); // ignored by single-GPU setups
    
    const auto ownershipInfo = vk::SemaphoreSubmitInfo{}
        .setSemaphore(_computeSyncs[currentFrame].ownershipSemaphore)
        .setStageMask(vk::PipelineStageFlagBits2::eAllCommands)
        .setValue(1).setDeviceIndex(0);

    const auto computeSubmitInfo = vk::SubmitInfo2{}
        .setCommandBufferInfos(computeInfo)
        .setSignalSemaphoreInfos(ownershipInfo);
    _computeQueue.submit2(computeSubmitInfo, _computeSyncs[currentFrame].computeDrawFence);
}

tpd::Engine::DrawPackage tpd::GaussianEngine::draw(const vk::Image image) {
    const auto currentFrame = _renderer->getCurrentDrawingFrame();
    const auto graphicsDraw = _drawingCommandBuffers[currentFrame];

    graphicsDraw.reset();
    graphicsDraw.begin(vk::CommandBufferBeginInfo{});

    if (_graphicsFamilyIndex == _computeFamilyIndex) {
        // Old contents from the previous frame will be cleared by the compute shader
        _targets[currentFrame].recordImageTransition(graphicsDraw, vk::ImageLayout::eGeneral);

        graphicsDraw.bindPipeline(vk::PipelineBindPoint::eCompute, _pipeline);
        graphicsDraw.bindDescriptorSets(
            vk::PipelineBindPoint::eCompute, _shaderLayout->getPipelineLayout(),
            /* first set */ 0, _shaderInstance->getDescriptorSets(currentFrame), {});

        const auto [w, h, _] = _targets[currentFrame].getPixelSize();
        graphicsDraw.dispatch(std::ceil(w / 16.0f), std::ceil(h / 16.0f), 1);

        _targets[currentFrame].recordImageTransition(graphicsDraw, vk::ImageLayout::eTransferSrcOptimal);
        foundation::recordLayoutTransition(
            graphicsDraw, image, vk::ImageAspectFlagBits::eColor, 1, 
            vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
        _targets[currentFrame].recordImageCopyTo(image, graphicsDraw);

        foundation::recordLayoutTransition(
            graphicsDraw, image, vk::ImageAspectFlagBits::eColor, 1, 
            vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR);
        
        graphicsDraw.end();
        return { graphicsDraw, vk::PipelineStageFlagBits2::eTransfer, vk::PipelineStageFlagBits2::eTransfer, {} };
    }

    // Async compute has drawn and released the image in preFramePass, acquire the released image from it
    // A Target image ensures its layout is TransferSrcOptimal after the ownership transfer
    _targets[currentFrame].recordOwnershipAcquire(graphicsDraw, _computeFamilyIndex, _graphicsFamilyIndex);

    foundation::recordLayoutTransition(
        graphicsDraw, image, vk::ImageAspectFlagBits::eColor, 1, 
        vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
    _targets[currentFrame].recordImageCopyTo(image, graphicsDraw);

    foundation::recordLayoutTransition(
        graphicsDraw, image, vk::ImageAspectFlagBits::eColor, 1, 
        vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR);
    graphicsDraw.end();

    constexpr vk::PipelineStageFlags2 ownershipWaitStage = vk::PipelineStageFlagBits2::eAllCommands;
    return { 
        graphicsDraw, vk::PipelineStageFlagBits2::eTransfer, vk::PipelineStageFlagBits2::eTransfer, 
        std::vector{ std::make_pair(_computeSyncs[currentFrame].ownershipSemaphore, ownershipWaitStage) } };
}

tpd::PhysicalDeviceSelection tpd::GaussianEngine::pickPhysicalDevice(
    const std::vector<const char*>& deviceExtensions,
    const vk::Instance instance,
    const vk::SurfaceKHR surface) const
{
    auto selector = PhysicalDeviceSelector()
        .requestComputeQueueFamily()
        .featuresVulkan13(getVulkan13Features());

    if (_renderer->hasSurfaceRenderingSupport()) {
        selector.requestGraphicsQueueFamily();
        selector.requestPresentQueueFamily(surface);
    }

    auto selection = selector.select(instance, deviceExtensions);

    // Modify the queue family indices to minimize queue ownership transfer operations
    if (_renderer->hasSurfaceRenderingSupport()) {
        // We found a distinct transfer queue, but since we're not going to use graphics much,
        // we can set transfer to graphics and avoid an additional ownership transfer to present 
        if (selection.graphicsQueueFamilyIndex == selection.presentQueueFamilyIndex &&
            selection.transferQueueFamilyIndex != selection.graphicsQueueFamilyIndex) {
            selection.transferQueueFamilyIndex = selection.graphicsQueueFamilyIndex;
        }
    }

    PLOGD << "Queue family indices selected:";
    PLOGD << " - Transfer: " << selection.transferQueueFamilyIndex;
    PLOGD << " - Compute:  " << selection.computeQueueFamilyIndex;
    if (_renderer->hasSurfaceRenderingSupport()) {
        PLOGD << " - Present:  " << selection.presentQueueFamilyIndex;
    }

    return selection;
}

vk::Device tpd::GaussianEngine::createDevice(
    const std::vector<const char*>& deviceExtensions,
    const std::initializer_list<uint32_t> queueFamilyIndices) const
{
    auto deviceFeatures = vk::PhysicalDeviceFeatures2{};
    auto featuresVulkan13 = getVulkan13Features();

    deviceFeatures.pNext = &featuresVulkan13;

    // Remember to update the count number at the end of the first message should more features are added
    PLOGD << "Device features requested by " << getName() << " (1):";
    PLOGD << " - Vulkan13Features: synchronization2";

    return DeviceBuilder()
        .deviceFeatures(&deviceFeatures)
        .queueFamilyIndices(queueFamilyIndices)
        .build(_physicalDevice, deviceExtensions);
}

vk::PhysicalDeviceVulkan13Features tpd::GaussianEngine::getVulkan13Features() {
    auto features = vk::PhysicalDeviceVulkan13Features();
    features.synchronization2 = true;
    return features;
}

void tpd::GaussianEngine::onInitialized() {
    PLOGD << "Assets directories used by " << getName() << ':';
    PLOGD << " - " << std::filesystem::path(TORPEDO_VOLUMETRIC_ASSETS_DIR);
    PLOGD << " - " << std::filesystem::path(TORPEDO_VOLUMETRIC_ASSETS_DIR) / "shaders";

    createRenderTargets();
    createPipelineResources();

    if (_graphicsFamilyIndex != _computeFamilyIndex) {
        createComputeCommandPool();
        createComputeCommandBuffers();
        createComputeSyncs();
    }
}

void tpd::GaussianEngine::createRenderTargets() {
    const auto targetBuilder = Target::Builder()
        .extent(_renderer->getFramebufferSize())
        .aspect(vk::ImageAspectFlagBits::eColor)
        .format(vk::Format::eR8G8B8A8Unorm)
        .usage(vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc);

    // Create a render target image for each in-flight frame
    const auto frameCount = _renderer->getInFlightFramesCount();
    _targets.reserve(frameCount);     // reserve only, so that we can move-construct later with push_back
    _targetViews.resize(frameCount);  // vk::ImageView can be copied

    for (auto i = 0; i < frameCount; ++i) {
        _targets.push_back(targetBuilder.build(*_deviceAllocator));
        _targetViews[i] = _targets[i].createImageView(vk::ImageViewType::e2D, _device);
    }

    PLOGD << "Number of target images created by " << getName() << ": " << _targets.size();
}

void tpd::GaussianEngine::createPipelineResources() {
    _shaderLayout = ShaderLayout::Builder(&_engineResourcePool)
        .descriptorSetCount(1)
        .descriptor(0, 0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute)
        .buildUnique(_device);

    _shaderInstance = _shaderLayout->createInstance(&_engineResourcePool, _device, _renderer->getInFlightFramesCount());
    for (uint32_t i = 0; i < _renderer->getInFlightFramesCount(); ++i) {
        const auto descriptorInfo = vk::DescriptorImageInfo{}
            .setImageView(_targetViews[i])
            .setImageLayout(vk::ImageLayout::eGeneral);
        _shaderInstance->setDescriptor(i, 0, 0, vk::DescriptorType::eStorageImage, _device, descriptorInfo);
    }

    const auto shaderModule = ShaderModuleBuilder()
        .shader(TORPEDO_VOLUMETRIC_ASSETS_DIR, "3dgs.comp")
        .build(_device);

    const auto shaderStage = vk::PipelineShaderStageCreateInfo{}
        .setModule(shaderModule)
        .setStage(vk::ShaderStageFlagBits::eCompute)
        .setPName("main");

    const auto pipelineInfo = vk::ComputePipelineCreateInfo{}
        .setStage(shaderStage)
        .setLayout(_shaderLayout->getPipelineLayout());
    _pipeline = _device.createComputePipeline(nullptr, pipelineInfo).value;

    _device.destroyShaderModule(shaderModule);
}

void tpd::GaussianEngine::createComputeCommandPool() {
    const auto poolInfo = vk::CommandPoolCreateInfo{}
        .setQueueFamilyIndex(_computeFamilyIndex)
        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    _computeCommandPool = _device.createCommandPool(poolInfo);
}

void tpd::GaussianEngine::createComputeCommandBuffers() {
    const auto allocInfo = vk::CommandBufferAllocateInfo{}
        .setCommandPool(_computeCommandPool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(_renderer->getInFlightFramesCount());

    const auto allocatedBuffers = _device.allocateCommandBuffers(allocInfo);
    _computeCommandBuffers.resize(allocatedBuffers.size());
    for (size_t i = 0; i < allocatedBuffers.size(); ++i) {
        _computeCommandBuffers[i] = allocatedBuffers[i];
    }
}

void tpd::GaussianEngine::createComputeSyncs() {
    _computeSyncs.resize(_renderer->getInFlightFramesCount());
    for (auto& [ownershipSemaphore, computeDrawFence] : _computeSyncs) {
        ownershipSemaphore = _device.createSemaphore({});
        computeDrawFence = _device.createFence(vk::FenceCreateInfo{ vk::FenceCreateFlagBits::eSignaled });
    }
}

void tpd::GaussianEngine::destroy() noexcept {
    if (_initialized) {
        if (_graphicsFamilyIndex != _computeFamilyIndex) {
            std::ranges::for_each(_computeSyncs, [this](const auto& sync) {
                _device.destroySemaphore(sync.ownershipSemaphore);
                _device.destroyFence(sync.computeDrawFence);
            });

            _computeCommandBuffers.clear();
            _device.destroyCommandPool(_computeCommandPool);
        }

        _device.destroyPipeline(_pipeline);

        _shaderInstance->destroy(_device);
        _shaderInstance.reset();

        _shaderLayout->destroy(_device);
        _shaderLayout.reset();

        std::ranges::for_each(_targetViews, [this](const auto it) { _device.destroyImageView(it); });
        _targets.clear();
    }
    Engine::destroy();
}
