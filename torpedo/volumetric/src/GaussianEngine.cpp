#include "torpedo/volumetric/GaussianEngine.h"

#include <torpedo/bootstrap/DeviceBuilder.h>
#include <torpedo/bootstrap/ShaderModuleBuilder.h>
#include <torpedo/foundation/ImageUtils.h>

#include <filesystem>
#include <numbers>

void tpd::GaussianEngine::preFrameCompute() {
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

    // The computeDrawFence already ensures that we're not resting the command buffer while it's still in use
    computeDraw.reset();
    computeDraw.begin(vk::CommandBufferBeginInfo{});

    // We can safely transition the image layout to General since a Target image ensures the proper synchronization
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
    cmd.dispatch((w + BLOCK_X - 1) / BLOCK_X, (h + BLOCK_Y - 1) / BLOCK_Y, 1);
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

    // Save the minimum uniform buffer alignment size
    _minUniformBufferOffsetAlignment = limits.minUniformBufferOffsetAlignment;

    _renderer->addFramebufferResizeCallback(this, framebufferResizeCallback);
    const auto [w, h] = _renderer->getFramebufferSize();

    createPointCloudObject();
    createCameraObject(w, h);

    createCameraBuffer();
    createGaussianPointBuffer();
    createRasterPointBuffer();

    createRenderTargets(w, h);

    createPreworkPipeline();
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
    createCameraObject(width, height);
    setTargetDescriptors();
}

void tpd::GaussianEngine::createPointCloudObject() {
    _pc = PointCloud{ GAUSSIAN_COUNT, 0 };
}

void tpd::GaussianEngine::createCameraObject(const uint32_t width, const uint32_t height) {
    _camera.imageSize = { width, height };
    _camera.position  = { 0.0f, 0.0f, -2.0f };

    const auto fovY = 60.0f * std::numbers::pi_v<float> / 180.0f; // radians
    const auto aspect = static_cast<float>(width) / static_cast<float>(height);
    _camera.tanFov.y  = std::numbers::inv_sqrt3_v<float>; // tan(60/2)
    _camera.tanFov.x  = _camera.tanFov.y * aspect;

    // Note that matrices in vsg are column-major, whereas Slang uses row-major. As long as we don't 
    // perform linear algebra arithmetics with vsg matrices, we can treat them as row-major.
    _camera.viewMatrix = vsg::mat4{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 2.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };

    // The projection map to reverse depth (1, 0) range, and the camera orientation is the same as OpenCV
    const auto fy = 1.f / std::tan(fovY / 2.f);
    const auto fx = fy / aspect;
    const auto za = NEAR / (NEAR - FAR);
    const auto zb = NEAR * FAR / (FAR - NEAR);
    const auto projection = vsg::mat4{
        fx,  0.f, 0.f, 0.f,
        0.f, fy,  0.f, 0.f,
        0.f, 0.f, za,  zb,
        0.f, 0.f, 1.f, 0.f
    };
    // Swap the multiplication order to account for column-major
    _camera.projMatrix = _camera.viewMatrix * projection;
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

void tpd::GaussianEngine::createCameraBuffer() {
    _cameraBuffer = RingBuffer::Builder()
        .count(_renderer->getInFlightFramesCount())
        .usage(vk::BufferUsageFlagBits::eUniformBuffer)
        .alloc(sizeof(Camera), Alignment::By_64)
        .build(*_deviceAllocator, &_engineResourcePool);
}

void tpd::GaussianEngine::createGaussianPointBuffer() {
    auto points = std::array<GaussianPoint, GAUSSIAN_COUNT>{};
    points[0].position   = vsg::vec3{ 0.0f, 0.0f, 0.0f };
    points[0].opacity    = 1.0f;
    points[0].quaternion = vsg::quat{ 0.0f, 0.0f, 0.0f, 1.0f };
    points[0].scale      = vsg::vec4{ 2.0f, 1.0f, 1.0f, 0.0f };
    // A gray Gaussian
    points[0].sh[0] = 2.0f;
    points[0].sh[1] = 2.0f;
    points[0].sh[2] = 2.0f;

    _gaussianPointBuffer = StorageBuffer::Builder()
        .usage(vk::BufferUsageFlagBits::eTransferDst)
        .alloc(sizeof(GaussianPoint) * GAUSSIAN_COUNT)
        .syncData(points.data())
        .dstPoint(vk::PipelineStageFlagBits2::eComputeShader, vk::AccessFlagBits2::eShaderStorageRead)
        .build(*_deviceAllocator, &_engineResourcePool);

    sync(*_gaussianPointBuffer, _computeFamilyIndex);
}

void tpd::GaussianEngine::createRasterPointBuffer() {
    _rasterPointBuffer = StorageBuffer::Builder()
        .alloc(RASTER_POINT_SIZE * GAUSSIAN_COUNT)
        .syncData(nullptr, RASTER_POINT_SIZE * GAUSSIAN_COUNT)
        .build(*_deviceAllocator, &_engineResourcePool);
}

void tpd::GaussianEngine::createPreworkPipeline() {
    _preworkLayout = ShaderLayout::Builder(&_engineResourcePool)
        .descriptorSetCount(1)
        .pushConstantRange(vk::ShaderStageFlagBits::eCompute, 0, sizeof(PointCloud))
        .descriptor(0, 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eCompute) // Camera
        .descriptor(0, 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute) // Gaussian points
        .descriptor(0, 2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute) // Raster points
        .buildUnique(_device);
    
    _preworkInstance = _preworkLayout->createInstance(&_engineResourcePool, _device, _renderer->getInFlightFramesCount());

    for (uint32_t i = 0; i < _renderer->getInFlightFramesCount(); ++i) {
        const auto descriptorInfo = vk::DescriptorBufferInfo{}
            .setBuffer(_cameraBuffer->getVulkanBuffer())
            .setOffset(_cameraBuffer->getOffset(i))
            .setRange(_cameraBuffer->getPerBufferSize());
        _preworkInstance->setDescriptor(i, 0, 0, vk::DescriptorType::eUniformBuffer, _device, descriptorInfo);
    }

    setStorageBufferDescriptors(*_gaussianPointBuffer, *_preworkInstance, 1);
    setStorageBufferDescriptors(*_rasterPointBuffer,   *_preworkInstance, 2);

    const auto shaderModule = ShaderModuleBuilder()
        .slang(TORPEDO_VOLUMETRIC_ASSETS_DIR, "prework.slang")
        .build(_device);

    const auto shaderStage = vk::PipelineShaderStageCreateInfo{}
        .setModule(shaderModule)
        .setStage(vk::ShaderStageFlagBits::eCompute)
        .setPName("main");
    
    const auto pipelineInfo = vk::ComputePipelineCreateInfo{}
        .setStage(shaderStage)
        .setLayout(_preworkLayout->getPipelineLayout());
    _preworkPipeline = _device.createComputePipeline(nullptr, pipelineInfo).value;

    _device.destroyShaderModule(shaderModule);
}

void tpd::GaussianEngine::createPipelineResources() {
    _shaderLayout = ShaderLayout::Builder(&_engineResourcePool)
        .descriptorSetCount(1)
        .descriptor(0, 0, vk::DescriptorType::eStorageImage,  1, vk::ShaderStageFlagBits::eCompute)
        .descriptor(0, 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute)
        .buildUnique(_device);

    _shaderInstance = _shaderLayout->createInstance(&_engineResourcePool, _device, _renderer->getInFlightFramesCount());
    setTargetDescriptors();
    setStorageBufferDescriptors(*_gaussianPointBuffer, *_shaderInstance, 1);

    const auto shaderModule = ShaderModuleBuilder()
        .slang(TORPEDO_VOLUMETRIC_ASSETS_DIR, "forward.slang")
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

void tpd::GaussianEngine::setStorageBufferDescriptors(
    const StorageBuffer& buffer,
    const ShaderInstance& instance,
    const uint32_t binding,
    const uint32_t set) const 
{
    for (uint32_t i = 0; i < _renderer->getInFlightFramesCount(); ++i) {
        const auto descriptorInfo = vk::DescriptorBufferInfo{}
            .setBuffer(buffer.getVulkanBuffer())
            .setOffset(0)
            .setRange(buffer.getSyncDataSize());
        instance.setDescriptor(i, set, binding, vk::DescriptorType::eStorageBuffer, _device, descriptorInfo);
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

        _device.destroyPipeline(_preworkPipeline);
        _device.destroyPipeline(_pipeline);

        _preworkInstance->destroy(_device);
        _preworkInstance.reset();
        _shaderInstance->destroy(_device);
        _shaderInstance.reset();

        _preworkLayout->destroy(_device);
        _preworkLayout.reset();
        _shaderLayout->destroy(_device);
        _shaderLayout.reset();

        cleanupRenderTargets();
        _targetViews.clear();

        _rasterPointBuffer.reset();
        _gaussianPointBuffer.reset();
        _cameraBuffer.reset();

        if (_renderer) {
            _renderer->removeFramebufferResizeCallback(this);
        }
    }
    Engine::destroy();
}
