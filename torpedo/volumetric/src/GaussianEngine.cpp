#include "torpedo/volumetric/GaussianEngine.h"

#include <torpedo/bootstrap/DeviceBuilder.h>
#include <torpedo/bootstrap/ShaderModuleBuilder.h>
#include <torpedo/foundation/ImageUtils.h>

#include <filesystem>
#include <numbers>

void tpd::GaussianEngine::preFrameCompute() {
    // Choose the right queue to submit pre-frame work
    const auto preFrameQueue = _graphicsFamilyIndex != _computeFamilyIndex ? _computeQueue : _graphicsQueue;

    const auto currentFrame = _renderer->getCurrentDrawingFrame();
    updateCameraBuffer(currentFrame);

    // Wait until the GPU has done with the pre-frame compute buffer for this frame
    using limits = std::numeric_limits<uint64_t>;
    [[maybe_unused]] const auto result = _device.waitForFences(_preFrameFences[currentFrame], vk::True, limits::max());
    _device.resetFences(_preFrameFences[currentFrame]);

    const auto preFrameCompute = _preFrameCommandBuffers[currentFrame];
    preFrameCompute.reset();
    preFrameCompute.begin(vk::CommandBufferBeginInfo{});

    // Preprocess dispatches these passes: prepare, prefix
    recordPreprocess(preFrameCompute, currentFrame);
    preFrameCompute.end();

    const auto preprocessInfo = vk::CommandBufferSubmitInfo{ preFrameCompute, 0b1 };
    const auto preprocessSubmitInfo = vk::SubmitInfo2{}.setCommandBufferInfos(preprocessInfo);
    preFrameQueue.submit2(preprocessSubmitInfo, _readBackFences[currentFrame]);

    // Wait until prefix has written _tilesRendered to the host visible buffer
    [[maybe_unused]] const auto _ = _device.waitForFences(_readBackFences[currentFrame], vk::True, limits::max());
    _device.resetFences(_readBackFences[currentFrame]);

    const auto tilesRendered = _tilesRenderedBuffer->read<uint32_t>();

    preFrameCompute.reset();
    preFrameCompute.begin(vk::CommandBufferBeginInfo{});

    recordFinalBlend(preFrameCompute, currentFrame);

    // Transfer ownership to graphics before submitting if working with async compute
    if (_graphicsFamilyIndex != _computeFamilyIndex) {
        _targets[currentFrame].recordOwnershipRelease(preFrameCompute, _computeFamilyIndex, _graphicsFamilyIndex);
    }

    preFrameCompute.end();

    const auto computeDrawInfo = vk::CommandBufferSubmitInfo{ preFrameCompute, 0b1 };
    auto computeDrawSubmitInfo = vk::SubmitInfo2{};
    computeDrawSubmitInfo.pCommandBufferInfos = &computeDrawInfo;
    computeDrawSubmitInfo.commandBufferInfoCount = 1;

    // Add an acquire semaphore for compute-graphics ownership transfer
    if (_graphicsFamilyIndex != _computeFamilyIndex) {
        const auto ownershipInfo = vk::SemaphoreSubmitInfo{}
            .setSemaphore(_ownershipSemaphores[currentFrame])
            .setStageMask(vk::PipelineStageFlagBits2::eAllCommands)
            .setValue(1).setDeviceIndex(0);

        computeDrawSubmitInfo.signalSemaphoreInfoCount = 1;
        computeDrawSubmitInfo.pSignalSemaphoreInfos = &ownershipInfo;
    }

    preFrameQueue.submit2(computeDrawSubmitInfo, _preFrameFences[currentFrame]);
}

tpd::Engine::DrawPackage tpd::GaussianEngine::draw(const vk::Image image) {
    const auto currentFrame = _renderer->getCurrentDrawingFrame();
    const auto graphicsDraw = _drawingCommandBuffers[currentFrame];

    graphicsDraw.reset();
    graphicsDraw.begin(vk::CommandBufferBeginInfo{});

    if (_graphicsFamilyIndex == _computeFamilyIndex) {
        // Transition draw target to transfer src and copy to the swap image
        _targets[currentFrame].recordImageTransition(graphicsDraw, vk::ImageLayout::eTransferSrcOptimal);
        recordTargetCopy(graphicsDraw, image, currentFrame);
        return { graphicsDraw, vk::PipelineStageFlagBits2::eTransfer, vk::PipelineStageFlagBits2::eTransfer, {} };
    }

    // Async compute has drawn and released the image in preFrameCompute, acquire the released image from it
    // A Target image ensures its layout is TransferSrcOptimal after the ownership transfer
    _targets[currentFrame].recordOwnershipAcquire(graphicsDraw, _computeFamilyIndex, _graphicsFamilyIndex);
    recordTargetCopy(graphicsDraw, image, currentFrame);

    constexpr vk::PipelineStageFlags2 ownershipWaitStage = vk::PipelineStageFlagBits2::eAllCommands;
    return { 
        graphicsDraw, vk::PipelineStageFlagBits2::eTransfer, vk::PipelineStageFlagBits2::eTransfer, 
        std::vector{ std::make_pair(_ownershipSemaphores[currentFrame], ownershipWaitStage) } };
}

void tpd::GaussianEngine::recordPreprocess(const vk::CommandBuffer cmd, const uint32_t currentFrame) const {
    // Prepare pass
    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, _preparePipeline);
    cmd.pushConstants(_prepareLayout->getPipelineLayout(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(PointCloud), &_pc);
    cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute, _prepareLayout->getPipelineLayout(),
        /* first set */ 0, _prepareInstance->getDescriptorSets(currentFrame), {});
    cmd.dispatch((GAUSSIAN_COUNT + LOCAL_SCAN - 1) / LOCAL_SCAN, 1, 1);

    // Wait until prepare pass has populated the raster point buffer
    auto barrier = vk::MemoryBarrier2{};
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eComputeShader;
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eComputeShader;
    barrier.srcAccessMask = vk::AccessFlagBits2::eShaderStorageWrite;
    barrier.dstAccessMask = vk::AccessFlagBits2::eShaderStorageRead;

    const auto dependencyInfo = vk::DependencyInfo{}.setMemoryBarriers(barrier);
    cmd.pipelineBarrier2(dependencyInfo);

    // Prefix pass
    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, _prefixPipeline);
    cmd.pushConstants(_prefixLayout->getPipelineLayout(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(PointCloud), &_pc);
    cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute, _prefixLayout->getPipelineLayout(),
        /* first set */ 0, _prefixInstance->getDescriptorSets(currentFrame), {});
    cmd.dispatch((GAUSSIAN_COUNT + LOCAL_SCAN - 1) / LOCAL_SCAN, 1, 1);
}

void tpd::GaussianEngine::recordFinalBlend(const vk::CommandBuffer cmd, const uint32_t currentFrame) {
    // We can safely transition the image layout to General since a Target image ensures synchronization
    // with transfer operations (image copy to graphics), and this synchronization is multi-queue safe.
    // In the case of async compute, we don't need ownership transfer from graphics to compute since
    // we don't care about the old content.
    _targets[currentFrame].recordImageTransition(cmd, vk::ImageLayout::eGeneral);

    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, _forwardPipeline);
    cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute, _forwardLayout->getPipelineLayout(),
        /* first set */ 0, _forwardInstance->getDescriptorSets(currentFrame), {});

    const auto [w, h, d] = _targets[currentFrame].getPixelSize();
    cmd.dispatch((w + BLOCK_X - 1) / BLOCK_X, (h + BLOCK_Y - 1) / BLOCK_Y, 1);
}

void tpd::GaussianEngine::recordTargetCopy(
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

    return selector.select(instance, deviceExtensions);
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
    // Log queue families
    PLOGD << "Queue family indices selected:";
    PLOGD << " - Compute:  " << _computeFamilyIndex;
    PLOGD << " - Transfer: " << _transferFamilyIndex;
    if (_renderer->hasSurfaceRenderingSupport()) {
        PLOGD << " - Graphics: " << _graphicsFamilyIndex;
        PLOGD << " - Present:  " << _presentFamilyIndex;
    }

    // Log relevant physical device capabilities
    const auto limits = _physicalDevice.getProperties().limits;
    const auto groupSize = limits.maxComputeWorkGroupCount;
    const auto localSize = limits.maxComputeWorkGroupSize;
    PLOGD << "Compute space limits:";
    PLOGD << " - Max work group: (" << groupSize[0] << ", " << groupSize[1] << ", " << groupSize[2] << ")";
    PLOGD << " - Max local size: (" << localSize[0] << ", " << localSize[1] << ", " << localSize[2] << ")";
    PLOGD << " - Max invocations: " << limits.maxComputeWorkGroupInvocations;
    PLOGD << " - Max shared size: " << limits.maxComputeSharedMemorySize / 1024 << "KB";

    PLOGD << "Storage buffer limits:";
    PLOGD << " - Max range: " << limits.maxStorageBufferRange / 1048576 << "MB";
    PLOGD << " - Min align: " << limits.minStorageBufferOffsetAlignment << "B";

    // Log location of the assets directory
    const auto assetsDir = std::filesystem::path(TORPEDO_VOLUMETRIC_ASSETS_DIR).make_preferred();
    PLOGD << "Assets directories used by " << getName() << ':';
    PLOGD << " - " << assetsDir / "slang";

    // The logic for resizing target and target view vectors should be made once here
    // since they can be recreated later and should not be resized again
    const auto frameCount = _renderer->getInFlightFramesCount();
    _targets.reserve(frameCount);     // only reserve here since the only way to append new Target is via push_back
    _targetViews.resize(frameCount);  // vk::ImageView can be copied

    _renderer->addFramebufferResizeCallback(this, framebufferResizeCallback);
    const auto [w, h] = _renderer->getFramebufferSize();

    createPointCloudObject(w, h);
    createCameraObject(w, h);
    createCameraBuffer();

    createGaussianBuffer();
    createSplatBuffer();
    createPrefixOffsetsBuffer();
    createTilesRenderedBuffer();

    createRenderTargets(w, h);

    createPreparePipeline();
    createPrefixPipeline();
    createForwardPipeline();

    if (_graphicsFamilyIndex != _computeFamilyIndex) {
        // Additional resources for async compute
        createComputeCommandPool();
        createOwnershipSemaphores();
    }

    createPreFrameCommandBuffers();
    createFences();
}

void tpd::GaussianEngine::framebufferResizeCallback(void* ptr, const uint32_t width, const uint32_t height) {
    const auto engine = static_cast<GaussianEngine*>(ptr);
    engine->onFramebufferResize(width, height);
}

void tpd::GaussianEngine::onFramebufferResize(const uint32_t width, const uint32_t height) {
    PLOGD << "Recreating render targets in tpd::GaussianEngine: " << width << ", " << height;

    cleanupRenderTargets();
    createRenderTargets(width, height);
    createPointCloudObject(width, height);
    createCameraObject(width, height);
    setTargetDescriptors();
}

void tpd::GaussianEngine::createPointCloudObject(const uint32_t width, const uint32_t height) {
    _pc = PointCloud{ GAUSSIAN_COUNT, 0, { width, height } };
}

void tpd::GaussianEngine::createCameraObject(const uint32_t width, const uint32_t height) {
    constexpr auto fovY = 60.0f * std::numbers::pi_v<float> / 180.0f; // radians
    const auto aspect = static_cast<float>(width) / static_cast<float>(height);
    _camera.tanFov.y  = std::numbers::inv_sqrt3_v<float>; // tan(60/2)
    _camera.tanFov.x  = _camera.tanFov.y * aspect;

    // Note that matrices in vsg are column-major, whereas Slang uses row-major
    _camera.viewMatrix = vsg::mat4{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 2.0f, 1.0f,
    };

    // The projection map to reverse depth (1, 0) range, and the camera orientation is the same as OpenCV
    const auto fy = 1.f / std::tan(fovY / 2.f);
    const auto fx = fy / aspect;
    constexpr auto za = NEAR / (NEAR - FAR);
    constexpr auto zb = NEAR * FAR / (FAR - NEAR);
    const auto projection = vsg::mat4{
        fx,  0.f, 0.f, 0.f,
        0.f, fy,  0.f, 0.f,
        0.f, 0.f, za,  1.f,
        0.f, 0.f, zb,  0.f,
    };

    _camera.projMatrix = projection * _camera.viewMatrix;
}

void tpd::GaussianEngine::createRenderTargets(const uint32_t width, const uint32_t height) {
    const auto targetBuilder = Target::Builder()
        .extent(width, height)
        .aspect(vk::ImageAspectFlagBits::eColor)
        .format(vk::Format::eR8G8B8A8Unorm)
        .usage(vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc);

    // Create a render target image and image view for each in-flight frame
    for (auto i = 0; i < _renderer->getInFlightFramesCount(); ++i) {
        _targets.push_back(targetBuilder.build(_deviceAllocator));
        _targetViews[i] = _targets[i].createImageView(vk::ImageViewType::e2D, _device);
    }

    PLOGD << "Number of target images created by " << getName() << ": " << _targets.size();
}

void tpd::GaussianEngine::createCameraBuffer() {
    // A RingBuffer internally uses offsets to manage the buffer, and for a uniform buffer, these offsets
    // must be aligned to the minUniformBufferOffsetAlignment limit of the physical device.
    const auto minAlignment = _physicalDevice.getProperties().limits.minUniformBufferOffsetAlignment;

    _cameraBuffer = RingBuffer::Builder()
        .count(_renderer->getInFlightFramesCount())
        .usage(vk::BufferUsageFlagBits::eUniformBuffer)
        .alloc(sizeof(Camera), minAlignment)
        .build(_deviceAllocator, &_engineResourcePool);
}

void tpd::GaussianEngine::updateCameraBuffer(const uint32_t currentFrame) const {
    _cameraBuffer->updateData(currentFrame, &_camera, sizeof(Camera));
}

void tpd::GaussianEngine::createGaussianBuffer() {
    auto points = std::array<Gaussian, GAUSSIAN_COUNT>{};
    points[0].position   = vsg::vec3{ 0.0f, 0.0f, 0.0f };
    points[0].opacity    = 1.0f;
    points[0].quaternion = vsg::quat{ 0.0f, 0.0f, 0.0f, 1.0f };
    points[0].scale      = vsg::vec4{ 0.2f, 0.1f, 0.1f, 1.0f };
    // A gray Gaussian
    points[0].sh[0] = 1.5f;
    points[0].sh[1] = 1.5f;
    points[0].sh[2] = 1.5f;

    _gaussianBuffer = StorageBuffer::Builder()
        .usage(vk::BufferUsageFlagBits::eTransferDst)
        .alloc(sizeof(Gaussian) * GAUSSIAN_COUNT)
        .build(_vmaAllocator);

    constexpr auto dstSync = SyncPoint{ vk::PipelineStageFlagBits2::eComputeShader, vk::AccessFlagBits2::eShaderStorageRead };
    transfer(points.data(), sizeof(Gaussian) * GAUSSIAN_COUNT, _gaussianBuffer, _computeFamilyIndex, dstSync);
}

void tpd::GaussianEngine::createSplatBuffer() {
    _splatBuffer = StorageBuffer::Builder()
        .alloc(SPLAT_SIZE * GAUSSIAN_COUNT)
        .build(_deviceAllocator, &_engineResourcePool);
}

void tpd::GaussianEngine::createPrefixOffsetsBuffer() {
    _prefixOffsetsBuffer = StorageBuffer::Builder()
        .alloc(sizeof(uint32_t) * GAUSSIAN_COUNT)
        .build(_deviceAllocator, &_engineResourcePool);
}

void tpd::GaussianEngine::createTilesRenderedBuffer() {
    _tilesRenderedBuffer = ReadbackBuffer::Builder()
        .usage(vk::BufferUsageFlagBits::eStorageBuffer)
        .alloc(sizeof(uint32_t))
        .build(_deviceAllocator, &_engineResourcePool);
}

void tpd::GaussianEngine::createPreparePipeline() {
    _prepareLayout = ShaderLayout::Builder(&_engineResourcePool)
        .descriptorSetCount(1)
        .pushConstantRange(vk::ShaderStageFlagBits::eCompute, 0, sizeof(PointCloud))
        .descriptor(0, 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eCompute) // Camera
        .descriptor(0, 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute) // Gaussian points
        .descriptor(0, 2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute) // Raster points
        .buildUnique(_device);
    
    _prepareInstance = _prepareLayout->createInstance(&_engineResourcePool, _device, _renderer->getInFlightFramesCount());
    _preparePipeline = buildComputePipeline("prepare.slang", _prepareLayout->getPipelineLayout());

    for (uint32_t i = 0; i < _renderer->getInFlightFramesCount(); ++i) {
        const auto descriptorInfo = vk::DescriptorBufferInfo{}
            .setBuffer(_cameraBuffer->getVulkanBuffer())
            .setOffset(_cameraBuffer->getOffset(i))
            .setRange(_cameraBuffer->getPerBufferSize());
        _prepareInstance->setDescriptor(i, 0, 0, vk::DescriptorType::eUniformBuffer, _device, descriptorInfo);
    }

    setStorageBufferDescriptors(_gaussianBuffer->getVulkanBuffer(), _gaussianBuffer->getSize(), *_prepareInstance, 1);
    setStorageBufferDescriptors(  _splatBuffer->getVulkanBuffer(), _splatBuffer->getSize(),   *_prepareInstance, 2);
}

void tpd::GaussianEngine::createPrefixPipeline() {
    _prefixLayout = ShaderLayout::Builder(&_engineResourcePool)
        .descriptorSetCount(1)
        .pushConstantRange(vk::ShaderStageFlagBits::eCompute, 0, sizeof(PointCloud))
        .descriptor(0, 0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute) // Raster points
        .descriptor(0, 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute) // prefix offsets
        .descriptor(0, 2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute) // tiles rendered
        .buildUnique(_device);

    _prefixInstance = _prefixLayout->createInstance(&_engineResourcePool, _device, _renderer->getInFlightFramesCount());
    _prefixPipeline = buildComputePipeline("prefix.slang", _prefixLayout->getPipelineLayout());

    setStorageBufferDescriptors(  _splatBuffer->getVulkanBuffer(),   _splatBuffer->getSize(), *_prefixInstance, 0);
    setStorageBufferDescriptors(_prefixOffsetsBuffer->getVulkanBuffer(), _prefixOffsetsBuffer->getSize(), *_prefixInstance, 1);
    setStorageBufferDescriptors(_tilesRenderedBuffer->getVulkanBuffer(), _tilesRenderedBuffer->getSize(), *_prefixInstance, 2);
}

void tpd::GaussianEngine::createForwardPipeline() {
    _forwardLayout = ShaderLayout::Builder(&_engineResourcePool)
        .descriptorSetCount(1)
        .descriptor(0, 0, vk::DescriptorType::eStorageImage,  1, vk::ShaderStageFlagBits::eCompute)
        .descriptor(0, 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute)
        .buildUnique(_device);

    _forwardInstance = _forwardLayout->createInstance(&_engineResourcePool, _device, _renderer->getInFlightFramesCount());
    _forwardPipeline = buildComputePipeline("forward.slang", _forwardLayout->getPipelineLayout());

    setTargetDescriptors();
    setStorageBufferDescriptors(_gaussianBuffer->getVulkanBuffer(), _gaussianBuffer->getSize(), *_forwardInstance, 1);
}

vk::Pipeline tpd::GaussianEngine::buildComputePipeline(const std::string& slangFile, const vk::PipelineLayout layout) const {
    const auto shaderModule = ShaderModuleBuilder()
        .slang(TORPEDO_VOLUMETRIC_ASSETS_DIR, slangFile)
        .build(_device);

    const auto shaderStage = vk::PipelineShaderStageCreateInfo{}
        .setModule(shaderModule)
        .setStage(vk::ShaderStageFlagBits::eCompute)
        .setPName("main");

    const auto pipelineInfo = vk::ComputePipelineCreateInfo{}
        .setStage(shaderStage)
        .setLayout(layout);
    const auto pipeline = _device.createComputePipeline(nullptr, pipelineInfo).value;

    _device.destroyShaderModule(shaderModule);
    return pipeline;
}

void tpd::GaussianEngine::setTargetDescriptors() const {
    for (uint32_t i = 0; i < _renderer->getInFlightFramesCount(); ++i) {
        const auto descriptorInfo = vk::DescriptorImageInfo{}
            .setImageView(_targetViews[i])
            .setImageLayout(vk::ImageLayout::eGeneral);
        _forwardInstance->setDescriptor(i, 0, 0, vk::DescriptorType::eStorageImage, _device, descriptorInfo);
    }
}

void tpd::GaussianEngine::setStorageBufferDescriptors(
    const vk::Buffer buffer,
    const std::size_t size,
    const ShaderInstance& instance,
    const uint32_t binding,
    const uint32_t set) const 
{
    for (uint32_t i = 0; i < _renderer->getInFlightFramesCount(); ++i) {
        const auto descriptorInfo = vk::DescriptorBufferInfo{}
            .setBuffer(buffer)
            .setOffset(0)
            .setRange(size);
        instance.setDescriptor(i, set, binding, vk::DescriptorType::eStorageBuffer, _device, descriptorInfo);
    }
}

void tpd::GaussianEngine::createComputeCommandPool() {
    const auto poolInfo = vk::CommandPoolCreateInfo{}
        .setQueueFamilyIndex(_computeFamilyIndex)
        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    _computeCommandPool = _device.createCommandPool(poolInfo);
}

void tpd::GaussianEngine::createOwnershipSemaphores() {
    _ownershipSemaphores.resize(_renderer->getInFlightFramesCount());
    for (auto i = 0; i < _renderer->getInFlightFramesCount(); ++i) {
        _ownershipSemaphores[i] = _device.createSemaphore({});
    }
}

void tpd::GaussianEngine::createPreFrameCommandBuffers() {
    // Preprocess command buffers handle these passes: prepare, prefix
    auto allocInfo = vk::CommandBufferAllocateInfo{};
    allocInfo.commandPool = _graphicsFamilyIndex == _computeFamilyIndex ? _drawingCommandPool : _computeCommandPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = _renderer->getInFlightFramesCount();

    const auto allocatedBuffers = _device.allocateCommandBuffers(allocInfo);
    _preFrameCommandBuffers.resize(allocatedBuffers.size());
    for (size_t i = 0; i < allocatedBuffers.size(); ++i) {
        _preFrameCommandBuffers[i] = allocatedBuffers[i];
    }
}

void tpd::GaussianEngine::createFences() {
    _preFrameFences.resize(_renderer->getInFlightFramesCount());
    _readBackFences.resize(_renderer->getInFlightFramesCount());
    for (auto i = 0; i < _renderer->getInFlightFramesCount(); ++i) {
        _preFrameFences[i] = _device.createFence(vk::FenceCreateInfo{ vk::FenceCreateFlagBits::eSignaled });
        _readBackFences[i] = _device.createFence({});
    }
}

void tpd::GaussianEngine::cleanupRenderTargets() noexcept {
    std::ranges::for_each(_targetViews, [this](const auto it) { _device.destroyImageView(it); });
    std::ranges::for_each(_targets, [](Target& it) { it.destroy(); });
    _targets.clear(); // the capacity remains the same, clear so that new Target can be correctly pushed back
}

void tpd::GaussianEngine::destroy() noexcept {
    if (_initialized) {
        std::ranges::for_each(_readBackFences, [this](const auto it) { _device.destroyFence(it); });
        std::ranges::for_each(_preFrameFences, [this](const auto it) { _device.destroyFence(it); });
        _readBackFences.clear();
        _preFrameFences.clear();

        _preFrameCommandBuffers.clear();

        if (_graphicsFamilyIndex != _computeFamilyIndex) {
            std::ranges::for_each(_ownershipSemaphores, [this](const auto it) { _device.destroySemaphore(it); });
            _device.destroyCommandPool(_computeCommandPool);
        }

        _device.destroyPipeline(_forwardPipeline);
        _forwardInstance->destroy(_device);
        _forwardInstance.reset();
        _forwardLayout->destroy(_device);
        _forwardLayout.reset();

        _device.destroyPipeline(_prefixPipeline);
        _prefixInstance->destroy(_device);
        _prefixInstance.reset();
        _prefixLayout->destroy(_device);
        _prefixLayout.reset();

        _device.destroyPipeline(_preparePipeline);
        _prepareInstance->destroy(_device);
        _prepareInstance.reset();
        _prepareLayout->destroy(_device);
        _prepareLayout.reset();

        cleanupRenderTargets();
        _targetViews.clear();

        _tilesRenderedBuffer.reset();
        _prefixOffsetsBuffer.reset();
        _splatBuffer.reset();
        _gaussianBuffer.reset();
        _cameraBuffer.reset();

        if (_renderer) {
            _renderer->removeFramebufferResizeCallback(this);
        }
    }
    Engine::destroy();
}
