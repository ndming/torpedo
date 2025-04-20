#include "torpedo/volumetric/GaussianEngine.h"
#include "torpedo/rendering/LogUtils.h"

#include <torpedo/bootstrap/DeviceBuilder.h>
#include <torpedo/bootstrap/ShaderModuleBuilder.h>
#include <torpedo/bootstrap/PhysicalDeviceSelector.h>

#include <filesystem>
#include <numbers>

tpd::PhysicalDeviceSelection tpd::GaussianEngine::pickPhysicalDevice(
    const std::vector<const char*>& deviceExtensions,
    const vk::Instance instance,
    const vk::SurfaceKHR surface) const
{
    auto selector = PhysicalDeviceSelector()
        .featuresVulkan13(getVulkan13Features());

    if (_renderer->supportSurfaceRendering()) {
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
#ifndef NDEBUG
    logDebugInfos();
#endif
    // We're going to need a transfer worker for data transfer
    _transferWorker = std::make_unique<TransferWorker>(
        _transferFamilyIndex, _graphicsFamilyIndex, _computeFamilyIndex, _physicalDevice, _device, _vmaAllocator);

    // Create queues relevant to Gaussian splatting
    _graphicsQueue = _device.getQueue(_graphicsFamilyIndex, 0);
    _computeQueue = _device.getQueue(_computeFamilyIndex, 0);

    // Resize render targets when the framebuffer resizes
    _renderer->addFramebufferResizeCallback(this, framebufferResizeCallback);

    createDrawingCommandPool();
    if (asyncCompute()) {
        createComputeCommandPool();
    }

    const auto frameCount = _renderer->getInFlightFrameCount();
    const auto [w, h] = _renderer->getFramebufferSize();

    // The logic for resizing frame and target view vectors should be made once here
    // since they can be recreated later and should not be resized again
    _frames.reserve(frameCount);
    _frames.resize(frameCount);
    _targetViews.resize(frameCount);

    createFrames();
    createRenderTargets(w, h);

    createPointCloudObject(w, h);
    createCameraObject(w, h);

    createCameraBuffer();
    createGaussianBuffer();
    createSplatBuffer();
    createPrefixOffsetsBuffer();
    createTilesRenderedBuffer();

    createPreparePipeline();
    createPrefixPipeline();
    createForwardPipeline();
}

void tpd::GaussianEngine::logDebugInfos() const noexcept {
    // Log queue families
    PLOGD << "Queue family indices selected:";
    PLOGD << " - Compute:  " << _computeFamilyIndex;
    PLOGD << " - Transfer: " << _transferFamilyIndex;
    if (_renderer->supportSurfaceRendering()) {
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

void tpd::GaussianEngine::createDrawingCommandPool() {
    const auto poolInfo = vk::CommandPoolCreateInfo{}
        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
        .setQueueFamilyIndex(_graphicsFamilyIndex);
    _drawingCommandPool = _device.createCommandPool(poolInfo);
}

void tpd::GaussianEngine::createComputeCommandPool() {
    const auto poolInfo = vk::CommandPoolCreateInfo{}
        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
        .setQueueFamilyIndex(_computeFamilyIndex);
    _computeCommandPool = _device.createCommandPool(poolInfo);
}

void tpd::GaussianEngine::createFrames() {
    const auto drawingAllocInfo = vk::CommandBufferAllocateInfo{ _drawingCommandPool, vk::CommandBufferLevel::ePrimary, 1 };
    const auto computeAllocInfo = vk::CommandBufferAllocateInfo{ _computeCommandPool, vk::CommandBufferLevel::ePrimary, 1 };

    for (auto& [drawing, compute, ownership, preFrameFence, readBackFence, _] : _frames) {
        drawing = _device.allocateCommandBuffers(drawingAllocInfo)[0];
        compute = _device.allocateCommandBuffers(asyncCompute()? computeAllocInfo : drawingAllocInfo)[0];
        preFrameFence = _device.createFence(vk::FenceCreateInfo{ vk::FenceCreateFlagBits::eSignaled });
        readBackFence = _device.createFence({});

        if (asyncCompute()) {
            ownership = _device.createSemaphore({});
        }
    }
}

void tpd::GaussianEngine::createRenderTargets(const uint32_t width, const uint32_t height) {
    const auto targetBuilder = Image::Builder<Target>()
        .extent(width, height)
        .format(vk::Format::eR8G8B8A8Unorm)
        .usage(vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc);

    // Create a render target image and image view for each in-flight frame
    for (auto i = 0; i < _renderer->getInFlightFrameCount(); ++i) {
        _frames[i].renderTarget = targetBuilder.build(_vmaAllocator);
        _targetViews[i] = _frames[i].renderTarget.createImageView(vk::ImageViewType::e2D, _device);
    }
}

void tpd::GaussianEngine::cleanupRenderTargets() noexcept {
    std::ranges::for_each(_targetViews, [this](const auto it) { _device.destroyImageView(it); });
    std::ranges::for_each(_frames, [this](Frame& it) { it.renderTarget.destroy(_vmaAllocator); });
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

void tpd::GaussianEngine::createCameraBuffer() {
    // A RingBuffer internally uses offsets to manage the buffer, and for a uniform buffer, these offsets
    // must be aligned to the minUniformBufferOffsetAlignment limit of the physical device.
    const auto minAlignment = _physicalDevice.getProperties().limits.minUniformBufferOffsetAlignment;

    _cameraBuffer = RingBuffer::Builder()
        .count(_renderer->getInFlightFrameCount())
        .usage(vk::BufferUsageFlagBits::eUniformBuffer)
        .alloc(sizeof(Camera), minAlignment)
        .build(_vmaAllocator);
}

void tpd::GaussianEngine::updateCameraBuffer(const uint32_t currentFrame) const {
    _cameraBuffer.update(currentFrame, &_camera, sizeof(Camera));
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
    _transferWorker->transfer(points.data(), sizeof(Gaussian) * GAUSSIAN_COUNT, _gaussianBuffer, _computeFamilyIndex, dstSync);
}

void tpd::GaussianEngine::createSplatBuffer() {
    _splatBuffer = StorageBuffer::Builder()
        .alloc(SPLAT_SIZE * GAUSSIAN_COUNT)
        .build(_vmaAllocator);
}

void tpd::GaussianEngine::createPrefixOffsetsBuffer() {
    _prefixOffsetsBuffer = StorageBuffer::Builder()
        .alloc(sizeof(uint32_t) * GAUSSIAN_COUNT)
        .build(_vmaAllocator);
}

void tpd::GaussianEngine::createTilesRenderedBuffer() {
    _tilesRenderedBuffer = ReadbackBuffer::Builder()
        .usage(vk::BufferUsageFlagBits::eStorageBuffer)
        .alloc(sizeof(uint32_t))
        .build(_vmaAllocator);
}

void tpd::GaussianEngine::createPreparePipeline() {
    _prepareLayout = ShaderLayout::Builder()
        .descriptorSetCount(1)
        .pushConstantRange(vk::ShaderStageFlagBits::eCompute, 0, sizeof(PointCloud))
        .descriptor(0, 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eCompute) // Camera
        .descriptor(0, 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute) // Gaussian points
        .descriptor(0, 2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute) // Raster points
        .build(_device, &_preparePipelineLayout);
    
    _prepareInstance = _prepareLayout.createInstance(_device, _renderer->getInFlightFrameCount());
    _preparePipeline = createPipeline("prepare.slang", _preparePipelineLayout);

    for (uint32_t i = 0; i < _renderer->getInFlightFrameCount(); ++i) {
        const auto descriptorInfo = vk::DescriptorBufferInfo{}
            .setBuffer(_cameraBuffer)
            .setOffset(_cameraBuffer.getOffset(i))
            .setRange(sizeof(Camera));
        _prepareInstance.setDescriptor(i, 0, 0, vk::DescriptorType::eUniformBuffer, _device, descriptorInfo);
    }

    setStorageBufferDescriptors(_gaussianBuffer, sizeof(Gaussian) * GAUSSIAN_COUNT, _prepareInstance, 1);
    setStorageBufferDescriptors(_splatBuffer, SPLAT_SIZE * GAUSSIAN_COUNT, _prepareInstance, 2);
}

void tpd::GaussianEngine::createPrefixPipeline() {
    _prefixLayout = ShaderLayout::Builder()
        .descriptorSetCount(1)
        .pushConstantRange(vk::ShaderStageFlagBits::eCompute, 0, sizeof(PointCloud))
        .descriptor(0, 0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute) // Raster points
        .descriptor(0, 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute) // prefix offsets
        .descriptor(0, 2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute) // tiles rendered
        .build(_device, &_prefixPipelineLayout);

    _prefixInstance = _prefixLayout.createInstance(_device, _renderer->getInFlightFrameCount());
    _prefixPipeline = createPipeline("prefix.slang", _prefixPipelineLayout);

    setStorageBufferDescriptors(_splatBuffer, SPLAT_SIZE * GAUSSIAN_COUNT, _prefixInstance, 0);
    setStorageBufferDescriptors(_prefixOffsetsBuffer, sizeof(uint32_t) * GAUSSIAN_COUNT, _prefixInstance, 1);
    setStorageBufferDescriptors(_tilesRenderedBuffer, sizeof(uint32_t), _prefixInstance, 2);
}

void tpd::GaussianEngine::createForwardPipeline() {
    _forwardLayout = ShaderLayout::Builder()
        .descriptorSetCount(1)
        .descriptor(0, 0, vk::DescriptorType::eStorageImage,  1, vk::ShaderStageFlagBits::eCompute)
        .descriptor(0, 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute)
        .build(_device, &_forwardPipelineLayout);

    _forwardInstance = _forwardLayout.createInstance(_device, _renderer->getInFlightFrameCount());
    _forwardPipeline = createPipeline("forward.slang", _forwardPipelineLayout);

    setTargetDescriptors();
    setStorageBufferDescriptors(_gaussianBuffer, sizeof(Gaussian) * GAUSSIAN_COUNT, _forwardInstance, 1);
}

vk::Pipeline tpd::GaussianEngine::createPipeline(const std::string& slangFile, const vk::PipelineLayout layout) const {
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
    for (uint32_t i = 0; i < _renderer->getInFlightFrameCount(); ++i) {
        const auto descriptorInfo = vk::DescriptorImageInfo{}
            .setImageView(_targetViews[i])
            .setImageLayout(vk::ImageLayout::eGeneral);
        _forwardInstance.setDescriptor(i, 0, 0, vk::DescriptorType::eStorageImage, _device, descriptorInfo);
    }
}

void tpd::GaussianEngine::setStorageBufferDescriptors(
    const vk::Buffer buffer,
    const std::size_t size,
    const ShaderInstance& instance,
    const uint32_t binding,
    const uint32_t set) const 
{
    for (uint32_t i = 0; i < _renderer->getInFlightFrameCount(); ++i) {
        const auto descriptorInfo = vk::DescriptorBufferInfo{}
            .setBuffer(buffer)
            .setOffset(0)
            .setRange(size);
        instance.setDescriptor(i, set, binding, vk::DescriptorType::eStorageBuffer, _device, descriptorInfo);
    }
}

void tpd::GaussianEngine::preFrameCompute() const {
    // Choose the right queue to submit pre-frame work
    const auto preFrameQueue = asyncCompute() ? _computeQueue : _graphicsQueue;

    const auto frameIndex = _renderer->getCurrentFrameIndex();
    updateCameraBuffer(frameIndex);

    // Wait until the GPU has done with the pre-frame compute buffer for this frame
    using limits = std::numeric_limits<uint64_t>;
    const auto preFrameFence = _frames[frameIndex].preFrameFence;
    [[maybe_unused]] const auto result = _device.waitForFences(preFrameFence, vk::True, limits::max());
    _device.resetFences(preFrameFence);

    const auto preFrameCompute = _frames[frameIndex].compute;
    preFrameCompute.reset();
    preFrameCompute.begin(vk::CommandBufferBeginInfo{});

    // Preprocess dispatches these passes: prepare, prefix
    recordPreprocess(preFrameCompute, frameIndex);
    preFrameCompute.end();

    const auto preprocessInfo = vk::CommandBufferSubmitInfo{ preFrameCompute, 0b1 };
    const auto preprocessSubmitInfo = vk::SubmitInfo2{}.setCommandBufferInfos(preprocessInfo);
    const auto readBackFence = _frames[frameIndex].readBackFence;
    preFrameQueue.submit2(preprocessSubmitInfo, readBackFence);

    // Wait until prefix has written _tilesRendered to the host visible buffer
    [[maybe_unused]] const auto _ = _device.waitForFences(readBackFence, vk::True, limits::max());
    _device.resetFences(readBackFence);

    const auto tilesRendered = _tilesRenderedBuffer.read<uint32_t>();

    preFrameCompute.reset();
    preFrameCompute.begin(vk::CommandBufferBeginInfo{});

    recordFinalBlend(preFrameCompute, frameIndex);

    // Transfer ownership to graphics before submitting if working with async compute
    if (asyncCompute()) {
        constexpr auto releaseSrcSync = SyncPoint{ vk::PipelineStageFlagBits2::eComputeShader, vk::AccessFlagBits2::eShaderStorageWrite };
        _frames[frameIndex].renderTarget.recordOwnershipRelease(
            preFrameCompute, _computeFamilyIndex, _graphicsFamilyIndex, releaseSrcSync,
            vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferSrcOptimal); // we're about to copy contents from target
    }

    preFrameCompute.end();

    const auto computeDrawInfo = vk::CommandBufferSubmitInfo{ preFrameCompute, 0b1 };
    auto computeDrawSubmitInfo = vk::SubmitInfo2{};
    computeDrawSubmitInfo.pCommandBufferInfos = &computeDrawInfo;
    computeDrawSubmitInfo.commandBufferInfoCount = 1;

    // Add an acquire semaphore for compute-graphics ownership transfer
    if (asyncCompute()) {
        const auto ownershipInfo = vk::SemaphoreSubmitInfo{}
            .setSemaphore(_frames[frameIndex].ownership)
            .setStageMask(vk::PipelineStageFlagBits2::eAllCommands)
            .setValue(1).setDeviceIndex(0);

        computeDrawSubmitInfo.signalSemaphoreInfoCount = 1;
        computeDrawSubmitInfo.pSignalSemaphoreInfos = &ownershipInfo;
    }

    preFrameQueue.submit2(computeDrawSubmitInfo, preFrameFence);
}

void tpd::GaussianEngine::draw(const SwapImage image) {
    const auto frameIndex = _renderer->getCurrentFrameIndex();
    const auto [imageReady, renderDone, frameDrawFence] = _renderer->getCurrentFrameSync();

    const auto graphicsDraw = _frames[frameIndex].drawing;

    // Prepare submit info to the graphics queue
    using PipelineStage = vk::PipelineStageFlagBits2;
    const auto bufferInfo = vk::CommandBufferSubmitInfo{ graphicsDraw, 0b1 };
    const auto doneInfo = vk::SemaphoreSubmitInfo{ renderDone, 1, PipelineStage::eTransfer, 0 };

    auto waitInfos = std::array<vk::SemaphoreSubmitInfo, 2>{};
    waitInfos[0].semaphore = imageReady;
    waitInfos[0].stageMask = PipelineStage::eTransfer;
    waitInfos[0].value = 1;
    waitInfos[0].deviceIndex = 0;
    waitInfos[1].semaphore = _frames[frameIndex].ownership;
    waitInfos[1].stageMask = PipelineStage::eAllCommands;
    waitInfos[1].value = 1;
    waitInfos[1].deviceIndex = 0;

    auto submitInfo = vk::SubmitInfo2{};
    submitInfo.commandBufferInfoCount = 1;
    submitInfo.pCommandBufferInfos = &bufferInfo;
    submitInfo.waitSemaphoreInfoCount = asyncCompute() ? 2 : 1;
    submitInfo.pWaitSemaphoreInfos = waitInfos.data();
    submitInfo.signalSemaphoreInfoCount = 1;
    submitInfo.pSignalSemaphoreInfos = &doneInfo;

    graphicsDraw.reset();
    graphicsDraw.begin(vk::CommandBufferBeginInfo{});

    using enum vk::ImageLayout;
    if (asyncCompute()) {
        // Async compute has drawn and released the image in preFrameCompute, acquire the released image from it
        constexpr auto acquireDstSync = SyncPoint{ PipelineStage::eTransfer, vk::AccessFlagBits2::eTransferRead };
        _frames[frameIndex].renderTarget.recordOwnershipAcquire(
            graphicsDraw, _computeFamilyIndex, _graphicsFamilyIndex, acquireDstSync,
            eGeneral, eTransferSrcOptimal); // we're about to copy contents from target
    } else {
        // Transition draw target to transfer src and copy its content to the swap image
        _frames[frameIndex].renderTarget.recordLayoutTransition(graphicsDraw, eGeneral, eTransferSrcOptimal);
    }

    recordTargetCopy(graphicsDraw, image, frameIndex);
    graphicsDraw.end();

    _graphicsQueue.submit2(submitInfo, frameDrawFence);
}

void tpd::GaussianEngine::recordPreprocess(const vk::CommandBuffer cmd, const uint32_t frameIndex) const noexcept {
    // Prepare pass
    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, _preparePipeline);
    cmd.pushConstants(_preparePipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(PointCloud), &_pc);
    cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute, _preparePipelineLayout,
        /* first set */ 0, _prepareInstance.getDescriptorSets(frameIndex), {});
    cmd.dispatch((GAUSSIAN_COUNT + LINEAR_WORKGROUP_SIZE - 1) / LINEAR_WORKGROUP_SIZE, 1, 1);

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
    cmd.pushConstants(_prefixPipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(PointCloud), &_pc);
    cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute, _prefixPipelineLayout,
        /* first set */ 0, _prefixInstance.getDescriptorSets(frameIndex), {});
    cmd.dispatch((GAUSSIAN_COUNT + LINEAR_WORKGROUP_SIZE - 1) / LINEAR_WORKGROUP_SIZE, 1, 1);
}

void tpd::GaussianEngine::recordFinalBlend(const vk::CommandBuffer cmd, const uint32_t frameIndex) const noexcept {
    // We can safely transition the image layout to General since a Target image ensures synchronization
    // with transfer operations (image copy to graphics), and this synchronization is multi-queue safe.
    // In the case of async compute, we don't need ownership transfer from graphics to compute since
    // we don't care about the old content.
    using enum vk::ImageLayout;
    const auto oldLayout = frameIndex < _renderer->getInFlightFrameCount() ? eUndefined : eTransferSrcOptimal;
    _frames[frameIndex].renderTarget.recordLayoutTransition(cmd, oldLayout, eGeneral);

    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, _forwardPipeline);
    cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute, _forwardPipelineLayout,
        /* first set */ 0, _forwardInstance.getDescriptorSets(frameIndex), {});

    const auto [w, h] = _renderer->getFramebufferSize();
    cmd.dispatch((w + BLOCK_X - 1) / BLOCK_X, (h + BLOCK_Y - 1) / BLOCK_Y, 1);
}

void tpd::GaussianEngine::recordTargetCopy(
    const vk::CommandBuffer cmd,
    const SwapImage swapImage,
    const uint32_t frameIndex) const noexcept
{
    using enum vk::ImageLayout;
    swapImage.recordLayoutTransition(cmd, eUndefined, eTransferDstOptimal);
    const auto [w, h] = _renderer->getFramebufferSize();
    _frames[frameIndex].renderTarget.recordDstImageCopy(cmd, swapImage, { w, h, 1 });
    swapImage.recordLayoutTransition(cmd, eTransferDstOptimal, ePresentSrcKHR);
}

void tpd::GaussianEngine::destroy() noexcept {
    if (_initialized) {
        _device.destroyPipeline(_forwardPipeline);
        _forwardInstance.destroy(_device);
        _forwardLayout.destroy(_device);
        _device.destroyPipelineLayout(_forwardPipelineLayout);

        _device.destroyPipeline(_prefixPipeline);
        _prefixInstance.destroy(_device);
        _prefixLayout.destroy(_device);
        _device.destroyPipelineLayout(_prefixPipelineLayout);

        _device.destroyPipeline(_preparePipeline);
        _prepareInstance.destroy(_device);
        _prepareLayout.destroy(_device);
        _device.destroyPipelineLayout(_preparePipelineLayout);

        _tilesRenderedBuffer.destroy(_vmaAllocator);
        _prefixOffsetsBuffer.destroy(_vmaAllocator);
        _splatBuffer.destroy(_vmaAllocator);
        _gaussianBuffer.destroy(_vmaAllocator);
        _cameraBuffer.destroy(_vmaAllocator);

        cleanupRenderTargets();
        std::ranges::for_each(_frames, [this](const Frame& f) {
            if (asyncCompute()) {
                _device.destroySemaphore(f.ownership);
            }
            _device.destroyFence(f.readBackFence);
            _device.destroyFence(f.preFrameFence);
        });
        _targetViews.clear();
        _frames.clear();

        if (asyncCompute()) {
            _device.destroyCommandPool(_computeCommandPool);
        }
        _device.destroyCommandPool(_drawingCommandPool);

        if (_renderer) {
            _renderer->removeFramebufferResizeCallback(this);
        }

        _transferWorker->destroy();
    }
    Engine::destroy();
}
