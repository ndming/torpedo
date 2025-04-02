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

    // Our computeDrawFence ensures that we're not resting the command buffer while it's still in use
    computeDraw.reset();
    computeDraw.begin(vk::CommandBufferBeginInfo{});

    // We can safely transition the image layout to General since a Target image ensures proper synchronization
    // with transfer operations (image copy in graphics), and this synchronization is multi-queue safe
    recordComputeDispatchCommands(computeDraw, currentFrame);

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
        recordComputeDispatchCommands(graphicsDraw, currentFrame);

        _targets[currentFrame].recordImageTransition(graphicsDraw, vk::ImageLayout::eTransferSrcOptimal);
        recordCopyToSwapImageCommands(graphicsDraw, image, currentFrame);

        return { graphicsDraw, vk::PipelineStageFlagBits2::eTransfer, vk::PipelineStageFlagBits2::eTransfer, {} };
    }

    // Async compute has drawn and released the image in preFramePass, acquire the released image from it
    // A Target image ensures its layout is TransferSrcOptimal after the ownership transfer
    _targets[currentFrame].recordOwnershipAcquire(graphicsDraw, _computeFamilyIndex, _graphicsFamilyIndex);
    recordCopyToSwapImageCommands(graphicsDraw, image, currentFrame);

    constexpr vk::PipelineStageFlags2 ownershipWaitStage = vk::PipelineStageFlagBits2::eAllCommands;
    return { 
        graphicsDraw, vk::PipelineStageFlagBits2::eTransfer, vk::PipelineStageFlagBits2::eTransfer, 
        std::vector{ std::make_pair(_computeSyncs[currentFrame].ownershipSemaphore, ownershipWaitStage) } };
}

void tpd::GaussianEngine::recordComputeDispatchCommands(const vk::CommandBuffer cmd, const uint32_t currentFrame) {
    // Old contents from the previous frame will be cleared by the compute shader
    // Also, we don't need ownership transfer from graphics to compute here since
    // we don't care about the old content
    _targets[currentFrame].recordImageTransition(cmd, vk::ImageLayout::eGeneral);

    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, _pipeline);
    cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute, _shaderLayout->getPipelineLayout(),
        /* first set */ 0, _shaderInstance->getDescriptorSets(currentFrame), {});

    const auto [w, h, d] = _targets[currentFrame].getPixelSize();
    cmd.dispatch(std::ceil(static_cast<float>(w) / 16.0f), std::ceil(static_cast<float>(h) / 16.0f), 1);
}

void tpd::GaussianEngine::recordCopyToSwapImageCommands(
    const vk::CommandBuffer cmd,
    const vk::Image swapImage,
    const uint32_t currentFrame) const
{
    foundation::recordLayoutTransition(
        cmd, swapImage, vk::ImageAspectFlagBits::eColor, 1, 
        vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

    _targets[currentFrame].recordImageCopyTo(swapImage, cmd);

    foundation::recordLayoutTransition(
        cmd, swapImage, vk::ImageAspectFlagBits::eColor, 1, 
        vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR);

    cmd.end();
}

tpd::PhysicalDeviceSelection tpd::GaussianEngine::pickPhysicalDevice(
    const std::vector<const char*>& deviceExtensions,
    const vk::Instance instance,
    const vk::SurfaceKHR surface) const
{
    auto selector = PhysicalDeviceSelector()
        .featuresVulkan13(getVulkan13Features());

    if (_renderer->hasSurfaceRenderingSupport()) {
        selector.requestGraphicsQueueFamily();
        selector.requestPresentQueueFamily(surface);
    }

    auto selection = selector.select(instance, deviceExtensions);

    PLOGD << "Queue family indices selected:";
    PLOGD << " - Compute:  " << selection.computeQueueFamilyIndex;
    PLOGD << " - Transfer: " << selection.transferQueueFamilyIndex;
    if (_renderer->hasSurfaceRenderingSupport()) {
        PLOGD << " - Graphics: " << selection.graphicsQueueFamilyIndex;
        PLOGD << " - Present:  " << selection.presentQueueFamilyIndex;
    }

    const auto limits = selection.physicalDevice.getProperties().limits;
    PLOGD << "Compute space limits:";
    PLOGD << " - Max work group x: " << limits.maxComputeWorkGroupCount[0];
    PLOGD << " - Max work group y: " << limits.maxComputeWorkGroupCount[1];
    PLOGD << " - Max work group z: " << limits.maxComputeWorkGroupCount[2];
    PLOGD << " - Max local size x: " << limits.maxComputeWorkGroupSize[0];
    PLOGD << " - Max local size y: " << limits.maxComputeWorkGroupSize[1];
    PLOGD << " - Max local size z: " << limits.maxComputeWorkGroupSize[2];
    PLOGD << " - Max invocations:  " << limits.maxComputeWorkGroupInvocations;
    PLOGD << " - Max group memory: " << limits.maxComputeSharedMemorySize / 1024 << "KB";

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
    const auto assetsDir = std::filesystem::path(TORPEDO_VOLUMETRIC_ASSETS_DIR).make_preferred();
    PLOGD << "Assets directories used by " << getName() << ':';
    PLOGD << " - " << assetsDir / "shaders";

    // The logic for resizing target and target view vectors should be made once here
    // since they can be recreated later and should not be resized again
    const auto frameCount = _renderer->getInFlightFramesCount();
    _targets.reserve(frameCount);     // only reserve here since the only way to append new Target is via push_back
    _targetViews.resize(frameCount);  // vk::ImageView can be copied

    _renderer->addFramebufferResizeCallback(this, framebufferResizeCallback);
    const auto [w, h] = _renderer->getFramebufferSize();
    createRenderTargets(w, h);
    createPointCloudBuffer();
    createPipelineResources();

    if (_graphicsFamilyIndex != _computeFamilyIndex) {
        // Additional resources for async compute
        createComputeCommandPool();
        createComputeCommandBuffers();
        createComputeSyncs();
    }
}

void tpd::GaussianEngine::framebufferResizeCallback(void* ptr, const uint32_t width, const uint32_t height) {
    const auto engine = static_cast<GaussianEngine*>(ptr);
    engine->onFramebufferResize(width, height);
}

void tpd::GaussianEngine::onFramebufferResize(const uint32_t width, const uint32_t height) {
    PLOGD << "Recreating render targets in tpd::GaussianEngine: " << width << ", " << height;

    cleanupRenderTargets();
    createRenderTargets(width, height);
    setTargetDescriptors();
}

void tpd::GaussianEngine::createRenderTargets(const uint32_t width, const uint32_t height) {
    const auto targetBuilder = Target::Builder()
        .extent(width, height)
        .aspect(vk::ImageAspectFlagBits::eColor)
        .format(vk::Format::eR8G8B8A8Unorm)
        .usage(vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc);

    // Create a render target image and image view for each in-flight frame
    for (auto i = 0; i < _renderer->getInFlightFramesCount(); ++i) {
        _targets.push_back(targetBuilder.build(*_deviceAllocator));
        _targetViews[i] = _targets[i].createImageView(vk::ImageViewType::e2D, _device);
    }

    PLOGD << "Number of target images created by " << getName() << ": " << _targets.size();
}

void tpd::GaussianEngine::createPointCloudBuffer() {
    auto points = std::array<GaussianPoint, 2>{};
    points[0].position   = { 1.0f, 0.0f, 0.0f };
    points[0].quaternion = { 0.0f, 1.0f, 0.0f, 0.0f };
    points[1].position   = { 0.0f, 0.0f, 1.0f };
    points[1].quaternion = { 0.5f, 0.5f, 0.5f, 0.0f };

    _pointCloudBuffer = StorageBuffer::Builder()
        .alloc(sizeof(GaussianPoint) * 2)
        .syncData(points.data())
        .dstPoint(vk::PipelineStageFlagBits2::eComputeShader, vk::AccessFlagBits2::eShaderStorageRead)
        .build(*_deviceAllocator, &_engineResourcePool);

    sync(*_pointCloudBuffer, _computeFamilyIndex);
}

void tpd::GaussianEngine::createPipelineResources() {
    _shaderLayout = ShaderLayout::Builder(&_engineResourcePool)
        .descriptorSetCount(1)
        .descriptor(0, 0, vk::DescriptorType::eStorageImage,  1, vk::ShaderStageFlagBits::eCompute)
        .descriptor(0, 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute)
        .buildUnique(_device);

    _shaderInstance = _shaderLayout->createInstance(&_engineResourcePool, _device, _renderer->getInFlightFramesCount());
    setTargetDescriptors();
    setBufferDescriptors();

    const auto shaderModule = ShaderModuleBuilder()
        .shader(TORPEDO_VOLUMETRIC_ASSETS_DIR, "3dgs.slang")
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

void tpd::GaussianEngine::setTargetDescriptors() const {
    for (uint32_t i = 0; i < _renderer->getInFlightFramesCount(); ++i) {
        const auto descriptorInfo = vk::DescriptorImageInfo{}
            .setImageView(_targetViews[i])
            .setImageLayout(vk::ImageLayout::eGeneral);
        _shaderInstance->setDescriptor(i, 0, 0, vk::DescriptorType::eStorageImage, _device, descriptorInfo);
    }
}

void tpd::GaussianEngine::setBufferDescriptors() const {
    for (uint32_t i = 0; i < _renderer->getInFlightFramesCount(); ++i) {
        const auto descriptorInfo = vk::DescriptorBufferInfo{}
            .setBuffer(_pointCloudBuffer->getVulkanBuffer())
            .setOffset(0)
            .setRange(_pointCloudBuffer->getSyncDataSize());
        _shaderInstance->setDescriptor(i, 0, 1, vk::DescriptorType::eStorageBuffer, _device, descriptorInfo);
    }
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

void tpd::GaussianEngine::cleanupRenderTargets() noexcept {
    std::ranges::for_each(_targetViews, [this](const auto it) { _device.destroyImageView(it); });
    std::ranges::for_each(_targets, [](Target& it) { it.destroy(); });
    _targets.clear(); // the capacity remains the same, clear so that new Target can be correctly pushed back
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

        _pointCloudBuffer.reset();

        cleanupRenderTargets();
        _targetViews.clear();

        if (_renderer) {
            _renderer->removeFramebufferResizeCallback(this);
        }
    }
    Engine::destroy();
}
