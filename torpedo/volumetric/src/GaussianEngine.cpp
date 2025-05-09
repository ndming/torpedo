#include "torpedo/volumetric/GaussianEngine.h"

#include <torpedo/bootstrap/DeviceBuilder.h>
#include <torpedo/bootstrap/ShaderModuleBuilder.h>
#include <torpedo/bootstrap/PhysicalDeviceSelector.h>

#include <plog/Log.h>

#include <filesystem>
#include <numbers>
#include <random>

tpd::PhysicalDeviceSelection tpd::GaussianEngine::pickPhysicalDevice(
    const std::vector<const char*>& deviceExtensions,
    const vk::Instance instance,
    const vk::SurfaceKHR surface) const
{
    auto selector = PhysicalDeviceSelector()
        .features(getFeatures())
        .featuresVulkan13(getVulkan13Features())
        .featuresShaderAtomicInt64(getShaderAtomicInt64Features());

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
    deviceFeatures.features = getFeatures();

    auto featuresVulkan13 = getVulkan13Features();
    deviceFeatures.pNext = &featuresVulkan13;

    auto featuresShaderAtomicInt64 = getShaderAtomicInt64Features();
    featuresVulkan13.pNext = &featuresShaderAtomicInt64;

    // Remember to update the count number at the end of the first message should more features are added
    PLOGD << "Device features requested by " << getName() << " (3):";
    PLOGD << " - Features: shaderInt64";
    PLOGD << " - Vulkan13Features: synchronization2";
    PLOGD << " - ShaderAtomicInt64Features: shaderBufferInt64Atomics";

    return DeviceBuilder()
        .deviceFeatures(&deviceFeatures)
        .queueFamilyIndices(queueFamilyIndices)
        .build(_physicalDevice, deviceExtensions);
}

vk::PhysicalDeviceFeatures tpd::GaussianEngine::getFeatures() {
    auto features = vk::PhysicalDeviceFeatures{};
    features.shaderInt64 = true;
    return features;
}

vk::PhysicalDeviceVulkan13Features tpd::GaussianEngine::getVulkan13Features() {
    auto features = vk::PhysicalDeviceVulkan13Features();
    features.synchronization2 = true;
    return features;
}

vk::PhysicalDeviceShaderAtomicInt64Features tpd::GaussianEngine::getShaderAtomicInt64Features() {
    auto features = vk::PhysicalDeviceShaderAtomicInt64Features{};
    features.shaderBufferInt64Atomics = true;
    return features;
}

void tpd::GaussianEngine::onInitialized() {
    // Log relevant physical device characteristics
    logDebugInfos();

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

    createGaussianLayout();
    _projectPipeline = createPipeline("project.slang", _gaussianLayout);
    _prefixPipeline = createPipeline("prefix.slang", _gaussianLayout);
    _keygenPipeline = createPipeline("keygen.slang", _gaussianLayout);
    _radixPipeline = createPipeline("radix.slang", _gaussianLayout);
    _coalescePipeline = createPipeline("coalesce.slang", _gaussianLayout);
    _rangePipeline = createPipeline("range.slang", _gaussianLayout);
    _forwardPipeline = createPipeline("forward.slang", _gaussianLayout);

    const auto frameCount = _renderer->getInFlightFrameCount();
    const auto [w, h] = _renderer->getFramebufferSize();

    // The logic for resizing frame and target view vectors should be made once here
    // since they can be recreated later and should not be resized again
    _frames.reserve(frameCount);
    _frames.resize(frameCount);
    _targetViews.resize(frameCount);

    createFrames();
    createRenderTargets(w, h);

    createPointCloudObject();
    createCameraObject(w, h);

    // These buffers can be used across in-flight frames due to the readback fence
    createCameraBuffer();
    createGaussianBuffer();
    createSplatBuffer();
    createTilesRenderedBuffer();
    createPartitionCountBuffer();
    createPartitionDescriptorBuffer();

    // These buffers are frame-dependent and the vectors should be resized once here
    // since they can be reallocated later and should not be resized again
    _splatKeyBuffers.resize(frameCount);
    _splatIndexBuffers.resize(frameCount);
    _blockCountBuffers.resize(frameCount);
    _blockDescriptorBuffer0s.resize(frameCount);
    _blockDescriptorBuffer1s.resize(frameCount);
    _globalSumBuffers.resize(frameCount);
    _globalPrefixBuffers.resize(frameCount);
    _tempKeyBuffers.resize(frameCount);
    _tempValBuffers.resize(frameCount);

    for (auto i = 0; i < frameCount; ++i) {
        createSplatKeyBuffer(i);
        createSplatIndexBuffer(i);
        createBlockDescriptorBuffers(i);
        createGlobalPrefixBuffer(i);
        createTempKeyBuffers(i);
        createTempValBuffers(i);
    }

    createBlockCountBuffers();
    createGlobalSumBuffers();
    createRangeBuffers(w, h);
}

void tpd::GaussianEngine::logDebugInfos() const noexcept {
    // Selected queue families
    PLOGD << "Queue family indices selected:";
    PLOGD << " - Compute:  " << _computeFamilyIndex;
    PLOGD << " - Transfer: " << _transferFamilyIndex;
    if (_renderer->supportSurfaceRendering()) {
        PLOGD << " - Graphics: " << _graphicsFamilyIndex;
        PLOGD << " - Present:  " << _presentFamilyIndex;
    }

    // Physical device capabilities
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

    // Assets directories
    const auto assetsDir = std::filesystem::path(TORPEDO_VOLUMETRIC_ASSETS_DIR).make_preferred();
    PLOGD << "Assets directories used by " << getName() << ':';
    PLOGD << " - " << assetsDir / "gaussian";
}

void tpd::GaussianEngine::framebufferResizeCallback(void* ptr, const uint32_t width, const uint32_t height) {
    const auto engine = static_cast<GaussianEngine*>(ptr);
    engine->onFramebufferResize(width, height);
}

void tpd::GaussianEngine::onFramebufferResize(const uint32_t width, const uint32_t height) {
    cleanupRenderTargets();
    createRenderTargets(width, height);

    createCameraObject(width, height);
    createRangeBuffers(width, height);

    PLOGD << "GaussianEngine - Recreated render targets and ranges buffer";

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

void tpd::GaussianEngine::createGaussianLayout() {
    using enum vk::DescriptorType;
    using enum vk::ShaderStageFlagBits;

    _shaderLayout = ShaderLayout<1>::Builder()
        .pushConstantRange(eCompute, 0, sizeof(PointCloud) + sizeof(uint32_t) + sizeof(uint32_t))
        .descriptor(0, 0, eStorageImage,  1, eCompute) // output image
        .descriptor(0, 1, eUniformBuffer, 1, eCompute) // camera
        .descriptor(0, 2, eStorageBuffer, 1, eCompute) // gaussians
        .descriptor(0, 3, eStorageBuffer, 1, eCompute) // splats
        .descriptor(0, 4, eStorageBuffer, 1, eCompute) // tiles rendered
        .descriptor(0, 5, eStorageBuffer, 1, eCompute) // partition count
        .descriptor(0, 6, eStorageBuffer, 1, eCompute) // partition descriptors
        .descriptor(0, 7, eStorageBuffer, 1, eCompute) // splat keys
        .descriptor(0, 8, eStorageBuffer, 1, eCompute) // splat indices
        .descriptor(0, 9, eStorageBuffer, 1, eCompute) // block count
        .descriptor(0,10, eStorageBuffer, 1, eCompute) // block descriptors 0
        .descriptor(0,11, eStorageBuffer, 1, eCompute) // block descriptors 1
        .descriptor(0,12, eStorageBuffer, 1, eCompute) // global sums
        .descriptor(0,13, eStorageBuffer, 1, eCompute) // global prefixes
        .descriptor(0,14, eStorageBuffer, 1, eCompute) // temp keys
        .descriptor(0,15, eStorageBuffer, 1, eCompute) // temp vals
        .descriptor(0,16, eStorageBuffer, 1, eCompute) // ranges
        .build(_device, &_gaussianLayout);
}

vk::Pipeline tpd::GaussianEngine::createPipeline(const std::string& slangFile, const vk::PipelineLayout layout) const {
    const auto shaderModule = ShaderModuleBuilder()
        .spirvPath(std::filesystem::path(TORPEDO_VOLUMETRIC_ASSETS_DIR) / "gaussian" / (slangFile + ".spv"))
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

void tpd::GaussianEngine::createFrames() {
    const auto drawingAllocInfo = vk::CommandBufferAllocateInfo{ _drawingCommandPool, vk::CommandBufferLevel::ePrimary, 1 };
    const auto computeAllocInfo = vk::CommandBufferAllocateInfo{ _computeCommandPool, vk::CommandBufferLevel::ePrimary, 1 };

    for (auto& [instance, drawing, compute, ownership, preFrameFence, readBackFence, maxTilesRendered, rangeBuffer, target] : _frames) {
        instance = _shaderLayout.createInstance(_device);
        drawing = _device.allocateCommandBuffers(drawingAllocInfo)[0];
        compute = _device.allocateCommandBuffers(asyncCompute()? computeAllocInfo : drawingAllocInfo)[0];
        preFrameFence = _device.createFence(vk::FenceCreateInfo{ vk::FenceCreateFlagBits::eSignaled });
        readBackFence = _device.createFence({});
        maxTilesRendered = 1;

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
        _frames[i].outputImage = targetBuilder.build(_vmaAllocator);
        _targetViews[i] = _frames[i].outputImage.createImageView(vk::ImageViewType::e2D, _device);
    }

    for (uint32_t i = 0; i < _renderer->getInFlightFrameCount(); ++i) {
        const auto descriptorInfo = vk::DescriptorImageInfo{}
            .setImageView(_targetViews[i])
            .setImageLayout(vk::ImageLayout::eGeneral);
        _frames[i].instance.setDescriptor(0, 0, vk::DescriptorType::eStorageImage, _device, descriptorInfo);
    }
}

void tpd::GaussianEngine::cleanupRenderTargets() noexcept {
    std::ranges::for_each(_targetViews, [this](const auto it) { _device.destroyImageView(it); });
    std::ranges::for_each(_frames, [this](Frame& it) { it.outputImage.destroy(_vmaAllocator); });
}

void tpd::GaussianEngine::createPointCloudObject() {
    _pc = PointCloud{ GAUSSIAN_COUNT, 0 };
}

void tpd::GaussianEngine::createCameraObject(const uint32_t width, const uint32_t height) {
    constexpr auto fovY = 60.0f * std::numbers::pi_v<float> / 180.0f; // radians
    const auto aspect = static_cast<float>(width) / static_cast<float>(height);
    _camera.tanFov.y  = std::numbers::inv_sqrt3_v<float>; // tan(60/2)
    _camera.tanFov.x  = _camera.tanFov.y * aspect;

    _camera.viewMatrix = mat4{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 2.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };

    // The projection map to reverse depth (1, 0) range, and the camera orientation is the same as OpenCV
    const auto fy = 1.f / std::tan(fovY / 2.f);
    const auto fx = fy / aspect;
    constexpr auto za = NEAR / (NEAR - FAR);
    constexpr auto zb = NEAR * FAR / (FAR - NEAR);
    const auto projection = mat4{
        fx,  0.f, 0.f, 0.f,
        0.f, fy,  0.f, 0.f,
        0.f, 0.f, za,  zb,
        0.f, 0.f, 1.f, 0.f,
    };
    _camera.projMatrix = math::mul(projection, _camera.viewMatrix);

    _camera.viewMatrix = math::transpose(_camera.viewMatrix);
    _camera.projMatrix = math::transpose(_camera.projMatrix);
}

void tpd::GaussianEngine::updateCameraBuffer(const uint32_t frameIndex) const {
    _cameraBuffer.update(frameIndex, &_camera, sizeof(Camera));
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

    // Set camera buffer descriptors
    for (uint32_t i = 0; i < _renderer->getInFlightFrameCount(); ++i) {
        const auto descriptorInfo = vk::DescriptorBufferInfo{}
            .setBuffer(_cameraBuffer)
            .setOffset(_cameraBuffer.getOffset(i))
            .setRange(sizeof(Camera));
        _frames[i].instance.setDescriptor(0, 1, vk::DescriptorType::eUniformBuffer, _device, descriptorInfo);
    }
}

void tpd::GaussianEngine::createGaussianBuffer() {
    auto points = std::vector<Gaussian>(GAUSSIAN_COUNT);

    std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution dist_pos(-2.0f, 2.0f);
    std::uniform_real_distribution dist_depth(-1.0f, 10.0f);
    std::uniform_real_distribution dist_opacity(0.1f, 1.0f);
    std::uniform_real_distribution dist_scale(0.008f, 0.08f);
    std::uniform_real_distribution dist_sh(0.0f, 1.5f);

    for (auto& [position, opacity, quaternion, scale, sh] : points) {
        position = { dist_pos(rng), dist_pos(rng), dist_depth(rng) };
        opacity = dist_opacity(rng);
        quaternion = { 0.0f, 0.0f, 0.0f, 1.0f };
        scale = { dist_scale(rng), dist_scale(rng), dist_scale(rng), 1.0f };
        sh[0] = dist_sh(rng);
        sh[1] = dist_sh(rng);
        sh[2] = dist_sh(rng);
    }

    _gaussianBuffer = StorageBuffer::Builder()
        .usage(vk::BufferUsageFlagBits::eTransferDst)
        .alloc(sizeof(Gaussian) * GAUSSIAN_COUNT)
        .build(_vmaAllocator);

    constexpr auto dstSync = SyncPoint{ vk::PipelineStageFlagBits2::eComputeShader, vk::AccessFlagBits2::eShaderStorageRead };
    _transferWorker->transfer(points.data(), sizeof(Gaussian) * GAUSSIAN_COUNT, _gaussianBuffer, _computeFamilyIndex, dstSync);
    points.clear(); // no longer need the points data

    setStorageBufferDescriptors(_gaussianBuffer, sizeof(Gaussian) * GAUSSIAN_COUNT, 2);
}

void tpd::GaussianEngine::createSplatBuffer() {
    _splatBuffer = StorageBuffer::Builder().alloc(SPLAT_SIZE * GAUSSIAN_COUNT).build(_vmaAllocator);
    setStorageBufferDescriptors(_splatBuffer, SPLAT_SIZE * GAUSSIAN_COUNT, 3);
}

void tpd::GaussianEngine::createTilesRenderedBuffer() {
    _tilesRenderedBuffer = ReadbackBuffer::Builder()
        .usage(vk::BufferUsageFlagBits::eStorageBuffer)
        .alloc(sizeof(uint32_t))
        .build(_vmaAllocator);

    setStorageBufferDescriptors(_tilesRenderedBuffer, sizeof(uint32_t), 4);
}

void tpd::GaussianEngine::createPartitionCountBuffer() {
    _partitionCountBuffer = StorageBuffer::Builder().alloc(sizeof(uint32_t)).build(_vmaAllocator);
    setStorageBufferDescriptors(_partitionCountBuffer, sizeof(uint32_t), 5);
}

void tpd::GaussianEngine::createPartitionDescriptorBuffer() {
    constexpr auto size = sizeof(uint64_t) * (GAUSSIAN_COUNT + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE;

    _partitionDescriptorBuffer = StorageBuffer::Builder().alloc(size).build(_vmaAllocator);
    setStorageBufferDescriptors(_partitionDescriptorBuffer, size, 6);
}

void tpd::GaussianEngine::createSplatKeyBuffer(const uint32_t frameIndex) {
    const auto size = sizeof(uint64_t) * _frames[frameIndex].maxTilesRendered;
    _splatKeyBuffers[frameIndex].destroy(_vmaAllocator);
    _splatKeyBuffers[frameIndex] = StorageBuffer::Builder().alloc(size).build(_vmaAllocator);

    const auto info = vk::DescriptorBufferInfo{}.setBuffer(_splatKeyBuffers[frameIndex]).setOffset(0).setRange(size);
    _frames[frameIndex].instance.setDescriptor(0, 7, vk::DescriptorType::eStorageBuffer, _device, info);
}

void tpd::GaussianEngine::createSplatIndexBuffer(const uint32_t frameIndex) {
    const auto size = sizeof(uint32_t) * _frames[frameIndex].maxTilesRendered;
    _splatIndexBuffers[frameIndex].destroy(_vmaAllocator);
    _splatIndexBuffers[frameIndex] = StorageBuffer::Builder().alloc(size).build(_vmaAllocator);

    const auto info = vk::DescriptorBufferInfo{}.setBuffer(_splatIndexBuffers[frameIndex]).setOffset(0).setRange(size);
    _frames[frameIndex].instance.setDescriptor(0, 8, vk::DescriptorType::eStorageBuffer, _device, info);
}

void tpd::GaussianEngine::createBlockCountBuffers() {
    const auto builder = StorageBuffer::Builder().alloc(sizeof(uint32_t));

    for (auto i = 0; i < _renderer->getInFlightFrameCount(); ++i) {
        _blockCountBuffers[i] = builder.build(_vmaAllocator);
        const auto info = vk::DescriptorBufferInfo{}.setBuffer(_blockCountBuffers[i]).setOffset(0).setRange(sizeof(uint32_t));
        _frames[i].instance.setDescriptor(0, 9, vk::DescriptorType::eStorageBuffer, _device, info);
    }
}

void tpd::GaussianEngine::createBlockDescriptorBuffers(const uint32_t frameIndex) {
    const auto size = sizeof(uint64_t) * (_frames[frameIndex].maxTilesRendered + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE;
    const auto builder = StorageBuffer::Builder().alloc(size);

    _blockDescriptorBuffer0s[frameIndex].destroy(_vmaAllocator);
    _blockDescriptorBuffer1s[frameIndex].destroy(_vmaAllocator);

    _blockDescriptorBuffer0s[frameIndex] = builder.build(_vmaAllocator);
    _blockDescriptorBuffer1s[frameIndex] = builder.build(_vmaAllocator);

    const auto info0 = vk::DescriptorBufferInfo{}.setBuffer(_blockDescriptorBuffer0s[frameIndex]).setOffset(0).setRange(size);
    const auto info1 = vk::DescriptorBufferInfo{}.setBuffer(_blockDescriptorBuffer1s[frameIndex]).setOffset(0).setRange(size);
    _frames[frameIndex].instance.setDescriptor(0, 10, vk::DescriptorType::eStorageBuffer, _device, info0);
    _frames[frameIndex].instance.setDescriptor(0, 11, vk::DescriptorType::eStorageBuffer, _device, info1);
}

void tpd::GaussianEngine::createGlobalSumBuffers() {
    constexpr auto size = sizeof(uint32_t) * 3; // see radix.slang
    const auto builder = StorageBuffer::Builder().alloc(size);

    for (auto i = 0; i < _renderer->getInFlightFrameCount(); ++i) {
        _globalSumBuffers[i] = builder.build(_vmaAllocator);
        const auto info = vk::DescriptorBufferInfo{}.setBuffer(_globalSumBuffers[i]).setOffset(0).setRange(size);
        _frames[i].instance.setDescriptor(0, 12, vk::DescriptorType::eStorageBuffer, _device, info);
    }
}

void tpd::GaussianEngine::createGlobalPrefixBuffer(const uint32_t frameIndex) {
    const auto size = sizeof(uint32_t) * _frames[frameIndex].maxTilesRendered;
    _globalPrefixBuffers[frameIndex].destroy(_vmaAllocator);
    _globalPrefixBuffers[frameIndex] = StorageBuffer::Builder().alloc(size).build(_vmaAllocator);

    const auto info = vk::DescriptorBufferInfo{}.setBuffer(_globalPrefixBuffers[frameIndex]).setOffset(0).setRange(size);
    _frames[frameIndex].instance.setDescriptor(0, 13, vk::DescriptorType::eStorageBuffer, _device, info);
}

void tpd::GaussianEngine::createTempKeyBuffers(const uint32_t frameIndex) {
    const auto size = sizeof(uint64_t) * _frames[frameIndex].maxTilesRendered;
    _tempKeyBuffers[frameIndex].destroy(_vmaAllocator);
    _tempKeyBuffers[frameIndex] = StorageBuffer::Builder().alloc(size).build(_vmaAllocator);

    const auto info = vk::DescriptorBufferInfo{}.setBuffer(_tempKeyBuffers[frameIndex]).setOffset(0).setRange(size);
    _frames[frameIndex].instance.setDescriptor(0, 14, vk::DescriptorType::eStorageBuffer, _device, info);
}

void tpd::GaussianEngine::createTempValBuffers(const uint32_t frameIndex) {
    const auto size = sizeof(uint32_t) * _frames[frameIndex].maxTilesRendered;
    _tempValBuffers[frameIndex].destroy(_vmaAllocator);
    _tempValBuffers[frameIndex] = StorageBuffer::Builder().alloc(size).build(_vmaAllocator);

    const auto info = vk::DescriptorBufferInfo{}.setBuffer(_tempValBuffers[frameIndex]).setOffset(0).setRange(size);
    _frames[frameIndex].instance.setDescriptor(0, 15, vk::DescriptorType::eStorageBuffer, _device, info);
}

void tpd::GaussianEngine::createRangeBuffers(const uint32_t width, const uint32_t height) {
    const auto tilesX = (width  + BLOCK_X - 1) / BLOCK_X;
    const auto tilesY = (height + BLOCK_Y - 1) / BLOCK_Y;
    const auto size = sizeof(uvec2) * tilesX * tilesY;

    // Also add transfer dst usage to clear the buffer without an additional compute pass
    const auto builder = StorageBuffer::Builder().usage(vk::BufferUsageFlagBits::eTransferDst).alloc(size);

    // Create a range buffer for each in-flight frame
    for (auto i = 0; i < _renderer->getInFlightFrameCount(); ++i) {
        // It's possible we're reallocating a new buffer due to image
        // dimension change, in which case the old one must be destroyed
        _frames[i].rangeBuffer.destroy(_vmaAllocator);
        _frames[i].rangeBuffer = builder.build(_vmaAllocator);

        const auto descriptorInfo = vk::DescriptorBufferInfo{}
            .setBuffer(_frames[i].rangeBuffer)
            .setOffset(0)
            .setRange(size);
        _frames[i].instance.setDescriptor(0, 16, vk::DescriptorType::eStorageBuffer, _device, descriptorInfo);
    }
}

void tpd::GaussianEngine::setStorageBufferDescriptors(
    const vk::Buffer buffer,
    const vk::DeviceSize size,
    const uint32_t binding) const
{
    for (uint32_t i = 0; i < _renderer->getInFlightFrameCount(); ++i) {
        const auto descriptorInfo = vk::DescriptorBufferInfo{}
            .setBuffer(buffer)
            .setOffset(0)
            .setRange(size);
        _frames[i].instance.setDescriptor(0, binding, vk::DescriptorType::eStorageBuffer, _device, descriptorInfo);
    }
}

void tpd::GaussianEngine::preFrameCompute() {
    // Choose the right queue to submit pre-frame work
    const auto preFrameQueue = asyncCompute() ? _computeQueue : _graphicsQueue;

    const auto frameIndex = _renderer->getCurrentFrameIndex();
    vmaSetCurrentFrameIndex(_vmaAllocator, frameIndex);

    // Set uniform buffers for camera
    updateCameraBuffer(frameIndex);

    // Wait until the GPU has done with the pre-frame compute buffer for this frame
    using limits = std::numeric_limits<uint64_t>;
    const auto preFrameFence = _frames[frameIndex].preFrameFence;
    [[maybe_unused]] const auto result = _device.waitForFences(preFrameFence, vk::True, limits::max());
    _device.resetFences(preFrameFence);

    const auto preFrameCompute = _frames[frameIndex].compute;
    preFrameCompute.reset();
    preFrameCompute.begin(vk::CommandBufferBeginInfo{});

    // Bind once before preprocess passes
    constexpr auto shaderStage = vk::ShaderStageFlagBits::eCompute;
    using enum vk::PipelineBindPoint;
    preFrameCompute.pushConstants(_gaussianLayout, shaderStage, 0, sizeof(PointCloud), &_pc);
    preFrameCompute.bindDescriptorSets(eCompute, _gaussianLayout, 0, _frames[frameIndex].instance.getDescriptorSets(), {});

    // Transition the render target to the format that can be inspected by the subsequent passes
    // We can safely transition the image layout to General since a Target image ensures synchronization
    // with transfer operations (image copy to graphics), and this synchronization is multi-queue safe.
    // If under async compute, we don't even need ownership transfer from graphics to compute since
    // we don't care about the old content.
    using enum vk::ImageLayout;
    _frames[frameIndex].outputImage.recordLayoutTransition(preFrameCompute, eUndefined, eGeneral);

    // Splat dispatches these passes: project, prefix
    recordSplat(preFrameCompute);
    preFrameCompute.end();

    const auto preprocessInfo = vk::CommandBufferSubmitInfo{ preFrameCompute, 0b1 };
    const auto preprocessSubmitInfo = vk::SubmitInfo2{}.setCommandBufferInfos(preprocessInfo);
    const auto readBackFence = _frames[frameIndex].readBackFence;
    preFrameQueue.submit2(preprocessSubmitInfo, readBackFence);

    // Wait until prefix has written _tilesRendered to the host visible buffer
    [[maybe_unused]] const auto _ = _device.waitForFences(readBackFence, vk::True, limits::max());
    _device.resetFences(readBackFence);

    // Inspect the number of tiles rendered and reallocate relevant buffers if necessary
    const auto tilesRendered = _tilesRenderedBuffer.read<uint32_t>();
    if (tilesRendered > _frames[frameIndex].maxTilesRendered) [[unlikely]] {
        _frames[frameIndex].maxTilesRendered = tilesRendered;
        reallocateBuffers(frameIndex);
    }

    preFrameCompute.reset();
    preFrameCompute.begin(vk::CommandBufferBeginInfo{});

    // Re-bind the layout and push the number of tiles rendered
    preFrameCompute.pushConstants(_gaussianLayout, shaderStage, 0,                  sizeof(PointCloud), &_pc);
    preFrameCompute.pushConstants(_gaussianLayout, shaderStage, sizeof(PointCloud), sizeof(uint32_t),   &tilesRendered);
    preFrameCompute.bindDescriptorSets(eCompute, _gaussianLayout, 0, _frames[frameIndex].instance.getDescriptorSets(), {});

    // The remaining passes: keygen, radix, range, forward
    recordBlend(preFrameCompute, tilesRendered, frameIndex);

    // Transfer ownership to graphics before submitting if working with async compute
    if (asyncCompute()) {
        constexpr auto releaseSrcSync = SyncPoint{ vk::PipelineStageFlagBits2::eComputeShader, vk::AccessFlagBits2::eShaderStorageWrite };
        _frames[frameIndex].outputImage.recordOwnershipRelease(
            preFrameCompute, _computeFamilyIndex, _graphicsFamilyIndex, releaseSrcSync,
            eGeneral, eTransferSrcOptimal); // we're about to copy contents from target
    }

    preFrameCompute.end();

    const auto computeDrawInfo = vk::CommandBufferSubmitInfo{ preFrameCompute, 0b1 };
    auto computeDrawSubmitInfo = vk::SubmitInfo2{};
    computeDrawSubmitInfo.pCommandBufferInfos = &computeDrawInfo;
    computeDrawSubmitInfo.commandBufferInfoCount = 1;

    // Acquire semaphore for compute-graphics ownership transfer, ...
    const auto ownershipInfo = vk::SemaphoreSubmitInfo{}
        .setSemaphore(_frames[frameIndex].ownership)
        .setStageMask(vk::PipelineStageFlagBits2::eAllCommands)
        .setValue(1).setDeviceIndex(0);
    // ... only add it if under async compute
    computeDrawSubmitInfo.signalSemaphoreInfoCount = asyncCompute() ? 1 : 0;
    computeDrawSubmitInfo.pSignalSemaphoreInfos = &ownershipInfo;

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
        _frames[frameIndex].outputImage.recordOwnershipAcquire(
            graphicsDraw, _computeFamilyIndex, _graphicsFamilyIndex, acquireDstSync,
            eGeneral, eTransferSrcOptimal); // we're about to copy contents from target
    } else {
        // Transition draw target to transfer src and copy its content to the swap image
        _frames[frameIndex].outputImage.recordLayoutTransition(graphicsDraw, eGeneral, eTransferSrcOptimal);
    }

    recordTargetCopy(graphicsDraw, image, frameIndex);
    graphicsDraw.end();

    _graphicsQueue.submit2(submitInfo, frameDrawFence);
}

void tpd::GaussianEngine::recordSplat(const vk::CommandBuffer cmd) const noexcept {
    // Project pass
    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, _projectPipeline);
    cmd.dispatch((GAUSSIAN_COUNT + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE, 1, 1);

    // Make sure splat contents written by project pass are visible (read),
    // and we're going to modify the tiles members in this buffer (write)
    // Global memory barrier covers all resources, generally considered 
    // more efficient to do a global memory barrier than per-resource barriers
    cmd.pipelineBarrier2(WAW_DEPENDENCY);

    // Prefix pass
    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, _prefixPipeline);
    cmd.dispatch((GAUSSIAN_COUNT + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE, 1, 1);
}

void tpd::GaussianEngine::reallocateBuffers(const uint32_t frameIndex) {
    createSplatKeyBuffer(frameIndex);
    createSplatIndexBuffer(frameIndex);
    createBlockDescriptorBuffers(frameIndex);
    createGlobalPrefixBuffer(frameIndex);
    createTempKeyBuffers(frameIndex);
    createTempValBuffers(frameIndex);

    PLOGD << "GaussianEngine - Frame " << frameIndex << " reallocated with new tiles rendered: " << _frames[frameIndex].maxTilesRendered;
}

void tpd::GaussianEngine::recordBlend(
    const vk::CommandBuffer cmd,
    const uint32_t tilesRendered,
    const uint32_t frameIndex) const noexcept 
{
    // Even with a fence staying in between, make sure prefix sums computed by prefix pass are visible
    cmd.pipelineBarrier2(RAW_DEPENDENCY);

    // Keygen pass
    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, _keygenPipeline);
    cmd.dispatch((GAUSSIAN_COUNT + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE, 1, 1);

    // Radix sort passes
    using enum vk::ShaderStageFlagBits;
    for (auto radixPass = 0; radixPass < 32; ++radixPass) {
        cmd.pushConstants(_gaussianLayout, eCompute, sizeof(PointCloud) + sizeof(uint32_t), sizeof(uint32_t), &radixPass);

        // Radix pass writes to the same set of buffers back-to-back
        cmd.pipelineBarrier2(RAW_DEPENDENCY);
        cmd.bindPipeline(vk::PipelineBindPoint::eCompute, _radixPipeline);
        cmd.dispatch((tilesRendered + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE, 1, 1);

        cmd.pipelineBarrier2(RAW_DEPENDENCY);
        cmd.bindPipeline(vk::PipelineBindPoint::eCompute, _coalescePipeline);
        cmd.dispatch((tilesRendered + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE, 1, 1);
    }

    // Clear the range buffer before populating it
    cmd.fillBuffer(_frames[frameIndex].rangeBuffer, 0, vk::WholeSize, 0);
    _frames[frameIndex].rangeBuffer.recordTransferDstPoint(cmd, { PipelineStage::eComputeShader, AccessMask::eShaderStorageWrite });

    // Make sure sorted keys written by radix pass are visible to range pass
    cmd.pipelineBarrier2(RAW_DEPENDENCY);

    // Range pass
    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, _rangePipeline);
    cmd.dispatch((tilesRendered + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE, 1, 1);

    // Make sure range values written by range pass are visible to forward pass
    cmd.pipelineBarrier2(RAW_DEPENDENCY);

    // Forward alpha blending pass
    const auto [w, h] = _renderer->getFramebufferSize();
    const auto tilesX = (w + BLOCK_X - 1) / BLOCK_X;
    const auto tilesY = (h + BLOCK_Y - 1) / BLOCK_Y;
    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, _forwardPipeline);
    cmd.dispatch(tilesX, tilesY, 1);
}

void tpd::GaussianEngine::recordTargetCopy(
    const vk::CommandBuffer cmd,
    const SwapImage swapImage,
    const uint32_t frameIndex) const noexcept
{
    using enum vk::ImageLayout;
    swapImage.recordLayoutTransition(cmd, eUndefined, eTransferDstOptimal);
    const auto [w, h] = _renderer->getFramebufferSize();
    _frames[frameIndex].outputImage.recordDstImageCopy(cmd, swapImage, { w, h, 1 });
    swapImage.recordLayoutTransition(cmd, eTransferDstOptimal, ePresentSrcKHR);
}

void tpd::GaussianEngine::destroy() noexcept {
    if (_initialized) {
        std::ranges::for_each(_frames, [this](Frame& f) { f.rangeBuffer.destroy(_vmaAllocator); });
        std::ranges::for_each(_tempValBuffers, [this](StorageBuffer& b) { b.destroy(_vmaAllocator); });
        std::ranges::for_each(_tempKeyBuffers, [this](StorageBuffer& b) { b.destroy(_vmaAllocator); });
        std::ranges::for_each(_globalPrefixBuffers, [this](StorageBuffer& b) { b.destroy(_vmaAllocator); });
        std::ranges::for_each(_globalSumBuffers, [this](StorageBuffer& b) { b.destroy(_vmaAllocator); });
        std::ranges::for_each(_blockDescriptorBuffer1s, [this](StorageBuffer& b) { b.destroy(_vmaAllocator); });
        std::ranges::for_each(_blockDescriptorBuffer0s, [this](StorageBuffer& b) { b.destroy(_vmaAllocator); });
        std::ranges::for_each(_blockCountBuffers, [this](StorageBuffer& b) { b.destroy(_vmaAllocator); });
        std::ranges::for_each(_splatIndexBuffers, [this](StorageBuffer& b) { b.destroy(_vmaAllocator); });
        std::ranges::for_each(_splatKeyBuffers, [this](StorageBuffer& b) { b.destroy(_vmaAllocator); });

        _globalSumBuffers.clear();
        _blockDescriptorBuffer1s.clear();
        _blockDescriptorBuffer0s.clear();
        _blockCountBuffers.clear();
        _splatIndexBuffers.clear();
        _splatKeyBuffers.clear();

        _partitionDescriptorBuffer.destroy(_vmaAllocator);
        _partitionCountBuffer.destroy(_vmaAllocator);
        _tilesRenderedBuffer.destroy(_vmaAllocator);
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
            f.instance.destroy(_device);
        });
        _targetViews.clear();
        _frames.clear();

        _device.destroyPipeline(_forwardPipeline);
        _device.destroyPipeline(_rangePipeline);
        _device.destroyPipeline(_coalescePipeline);
        _device.destroyPipeline(_radixPipeline);
        _device.destroyPipeline(_keygenPipeline);
        _device.destroyPipeline(_prefixPipeline);
        _device.destroyPipeline(_projectPipeline);

        _shaderLayout.destroy(_device);
        _device.destroyPipelineLayout(_gaussianLayout);

        if (asyncCompute()) {
            _device.destroyCommandPool(_computeCommandPool);
        }
        _device.destroyCommandPool(_drawingCommandPool);

        if (_renderer) {
            _renderer->removeFramebufferResizeCallback(this);
        }

        _transferWorker->destroy();
        _transferWorker.reset();
    }
    Engine::destroy();
}
