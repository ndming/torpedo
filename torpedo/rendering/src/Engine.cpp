#include "torpedo/rendering/Engine.h"
#include "torpedo/rendering/Utils.h"

#include <torpedo/bootstrap/DebugUtils.h>
#include <torpedo/bootstrap/DeviceBuilder.h>
#include <torpedo/foundation/StagingBuffer.h>

#include <plog/Log.h>

void tpd::Engine::init(
    const vk::Instance instance,
    const vk::SurfaceKHR surface,
    std::vector<const char*>&& rendererDeviceExtensions)
{
    auto extensions = getDeviceExtensions();
    extensions.insert(extensions.end(),
        std::make_move_iterator(rendererDeviceExtensions.begin()),
        std::make_move_iterator(rendererDeviceExtensions.end()));

    // Init physical device and create the logical device
    const auto [physicalDevice, graphicsIndex, transferIndex, presentIndex, computeIndex] = pickPhysicalDevice(extensions, instance, surface);
    _physicalDevice = physicalDevice;
    _device = createDevice(extensions, { graphicsIndex, transferIndex, computeIndex, presentIndex });

    PLOGI << "Found a suitable device for " << getName() << ": " << _physicalDevice.getProperties().deviceName.data();

    // Init all queue-related info
    _graphicsFamilyIndex = graphicsIndex;
    _transferFamilyIndex = transferIndex;
    _computeFamilyIndex  = computeIndex;
    _presentFamilyIndex  = presentIndex;

    _graphicsQueue = _device.getQueue(_graphicsFamilyIndex, 0);
    _transferQueue = _device.getQueue(_transferFamilyIndex, 0);
    _computeQueue  = _device.getQueue(_computeFamilyIndex, 0);

#ifndef NDEBUG
    auto asyncOperations = 0;
    if (_graphicsFamilyIndex != _transferFamilyIndex) asyncOperations++;
    if (_graphicsFamilyIndex != _computeFamilyIndex)  asyncOperations++;
    const auto countStr = asyncOperations == 0 ? "none)" : std::to_string(asyncOperations) + "):";
    PLOGD << "Async operations supported by the device (" << countStr;
    if (_graphicsFamilyIndex != _transferFamilyIndex) {
        PLOGD << "- Transfer";
    }
    if (_graphicsFamilyIndex != _computeFamilyIndex) {
        PLOGD << "- Compute";
    }

    bootstrap::setVulkanObjectName(
        static_cast<VkPhysicalDevice>(_physicalDevice), vk::ObjectType::ePhysicalDevice,
        getName() + std::string{ " - PhysicalDevice" }, instance, _device);

    bootstrap::setVulkanObjectName(
        static_cast<VkDevice>(_device), vk::ObjectType::eDevice,
        getName() + std::string{ " - Device" }, instance, _device);

    bootstrap::setVulkanObjectName(
        static_cast<VkQueue>(_graphicsQueue), vk::ObjectType::eQueue,
        getName() + std::string{ " - GraphicsQueue" }, instance, _device);

    bootstrap::setVulkanObjectName(
        static_cast<VkQueue>(_transferQueue), vk::ObjectType::eQueue,
        getName() + std::string{ " - TransferQueue" }, instance, _device);

    bootstrap::setVulkanObjectName(
        static_cast<VkQueue>(_computeQueue), vk::ObjectType::eQueue,
        getName() + std::string{ " - ComputeQueue" }, instance, _device);
#endif
    // Create command pools
    createDrawingCommandPool();
    createStartupCommandPools();

    // Drawing command buffers used by downstream
    createDrawingCommandBuffers();
    PLOGD << "Number of drawing command buffers created: " << _drawingCommandBuffers.size();

    // Create a device allocator
    _deviceAllocator = DeviceAllocator::Builder()
        .flags(VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT | VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT)
        .vulkanApiVersion(VK_API_VERSION_1_3)
        .build(instance, _physicalDevice, _device, &_memoryPool);

    PLOGI << "Using VMA API version: 1.3";
    PLOGD << "VMA created with the following flags (2):";
    PLOGD << " - VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT";
    PLOGD << " - VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT";

    // Tell downstream to init their resources
    onInitialized();
}

std::vector<const char*> tpd::Engine::getDeviceExtensions() const {
    auto extensions = std::vector{
        vk::EXTMemoryBudgetExtensionName,    // help VMA estimate memory budget more accurately
        vk::EXTMemoryPriorityExtensionName,  // incorporate memory priority to the allocator
    };
    rendering::logDebugExtensions("Device", Engine::getName(), extensions);
    return extensions;
}

tpd::PhysicalDeviceSelection tpd::Engine::pickPhysicalDevice(
    const std::vector<const char*>& deviceExtensions,
    const vk::Instance instance,
    const vk::SurfaceKHR surface) const
{
    auto selector = PhysicalDeviceSelector()
        .requestComputeQueueFamily();

    const auto requestingPresentFamily = static_cast<VkSurfaceKHR>(surface) != VK_NULL_HANDLE;
    if (requestingPresentFamily) {
        selector.requestPresentQueueFamily(surface);
    }

    const auto selection = selector.select(instance, deviceExtensions);
    PLOGD << "Queue family indices selected:";
    PLOGD << " - Graphics: " << selection.graphicsQueueFamilyIndex;
    PLOGD << " - Transfer: " << selection.transferQueueFamilyIndex;
    if (requestingPresentFamily) {
        PLOGD << " - Present:  " << selection.presentQueueFamilyIndex;
    }

    return selection;
}

vk::Device tpd::Engine::createDevice(
    const std::vector<const char*>& deviceExtensions,
    const std::initializer_list<uint32_t> queueFamilyIndices) const
{
    return DeviceBuilder()
        .queueFamilyIndices(queueFamilyIndices)
        .build(_physicalDevice, deviceExtensions);
}

void tpd::Engine::createDrawingCommandPool() {
    // This pool allocates command buffers for drawing commands
    const auto poolInfo = vk::CommandPoolCreateInfo{}
        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
        .setQueueFamilyIndex(_graphicsFamilyIndex);
    _drawingCommandPool = _device.createCommandPool(poolInfo);
}

void tpd::Engine::createStartupCommandPools() {
    // These pools allocate command buffers for one-shot transfers at the start of the program
    _graphicsCommandPool = _device.createCommandPool({ vk::CommandPoolCreateFlagBits::eTransient, _graphicsFamilyIndex });
    _transferCommandPool = _device.createCommandPool({ vk::CommandPoolCreateFlagBits::eTransient, _transferFamilyIndex });
}

void tpd::Engine::createDrawingCommandBuffers() {
    auto allocInfo = vk::CommandBufferAllocateInfo{};
    allocInfo.commandPool = _drawingCommandPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = _renderer->getInFlightFramesCount();
    _drawingCommandBuffers = _device.allocateCommandBuffers(allocInfo);
}

vk::CommandBuffer tpd::Engine::beginOneTimeTransfer(const vk::CommandPool commandPool) {
    const auto allocInfo = vk::CommandBufferAllocateInfo{}
        .setCommandPool(commandPool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(1);

    // The command pool might be accessed from the deletion worker thread
    std::lock_guard lock(_startupCommandPoolMutex);
    const auto buffer = _device.allocateCommandBuffers(allocInfo)[0];

    constexpr auto beginInfo = vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
    buffer.begin(beginInfo);
    return buffer;
}

void tpd::Engine::endOneTimeTransfer(const vk::CommandBuffer buffer, const vk::Fence deletionFence) const {
    buffer.end();
    _transferQueue.submit(vk::SubmitInfo{ {}, {}, {}, 1, &buffer }, deletionFence);
}

vk::SemaphoreSubmitInfo tpd::Engine::createOwnershipSemaphoreInfo() const {
    const auto ownershipTransferSemaphore = _device.createSemaphore({});
    auto semaphoreInfo = vk::SemaphoreSubmitInfo{};
    semaphoreInfo.semaphore = ownershipTransferSemaphore;
    // The only valid wait stage for ownership transfer is all-command
    semaphoreInfo.stageMask = vk::PipelineStageFlagBits2::eAllCommands;
    semaphoreInfo.deviceIndex = 0;
    semaphoreInfo.value = 1;
    return semaphoreInfo;
}

void tpd::Engine::endReleaseCommands(const vk::CommandBuffer buffer, const vk::SemaphoreSubmitInfo& semaphoreInfo) const {
    buffer.end();

    auto releaseInfo = vk::CommandBufferSubmitInfo{};
    releaseInfo.commandBuffer = buffer;
    releaseInfo.deviceMask = 0b1;  // ignored by single-GPU setups

    const auto releaseSubmitInfo = vk::SubmitInfo2{}
        .setCommandBufferInfos(releaseInfo)
        .setSignalSemaphoreInfos(semaphoreInfo);
    _transferQueue.submit2(releaseSubmitInfo);
}

void tpd::Engine::endAcquireCommands(
    const vk::CommandBuffer buffer,
    const vk::SemaphoreSubmitInfo& semaphoreInfo,
    const vk::Fence deletionFence) const
{
    buffer.end();

    auto acquireInfo = vk::CommandBufferSubmitInfo{};
    acquireInfo.commandBuffer = buffer;
    acquireInfo.deviceMask = 0b1;  // ignored by single-GPU setups

    const auto acquireSubmitInfo = vk::SubmitInfo2{}
        .setCommandBufferInfos(acquireInfo)
        .setWaitSemaphoreInfos(semaphoreInfo);
    _graphicsQueue.submit2(acquireSubmitInfo, deletionFence);
}

void tpd::Engine::sync(const StorageBuffer& storageBuffer) {
    if (!storageBuffer.hasSyncData()) {
        PLOGW << "Engine - Syncing an empty StorageBuffer: did you forget to call StorageBuffer::setSyncData()?";
        return;
    }

    auto stagingBuffer = StagingBuffer::Builder()
        .alloc(storageBuffer.getSyncDataSize())
        .build(*_deviceAllocator, &_memoryPool);
    stagingBuffer->setData(storageBuffer.getSyncData());

    const auto releaseCommand = beginOneTimeTransfer(_transferCommandPool);
    storageBuffer.recordBufferTransfer(releaseCommand, stagingBuffer->getVulkanBuffer());

    // Signals the fence after all queue jobs finish, allowing resource cleanup in the deletion worker's thread
    const auto deletionFence = _device.createFence(vk::FenceCreateInfo{});

    if (_transferFamilyIndex != _graphicsFamilyIndex) {
        storageBuffer.recordOwnershipRelease(releaseCommand, _transferFamilyIndex, _graphicsFamilyIndex);
        const auto ownershipSemaphoreInfo = createOwnershipSemaphoreInfo();
        endReleaseCommands(releaseCommand, ownershipSemaphoreInfo);

        const auto acquireCommand = beginOneTimeTransfer(_graphicsCommandPool);
        storageBuffer.recordOwnershipAcquire(acquireCommand, _transferFamilyIndex, _graphicsFamilyIndex);
        endAcquireCommands(acquireCommand, ownershipSemaphoreInfo, deletionFence);

        _stagingDeletionQueue.submit(
            _device, ownershipSemaphoreInfo.semaphore, deletionFence, std::move(stagingBuffer),
            { std::pair{ _graphicsCommandPool, acquireCommand }, std::pair{ _transferCommandPool, releaseCommand } });

    } else {
        endOneTimeTransfer(releaseCommand, deletionFence);
        _stagingDeletionQueue.submit(_device, {}, deletionFence, std::move(stagingBuffer), {{ _transferCommandPool, releaseCommand }});
    }
}

void tpd::Engine::sync(Texture& texture) {
    if (!texture.hasSyncData()) {
        PLOGW << "Engine - Syncing an empty Texture: did you forget to call Texture::setSyncData()?";
        return;
    }

    auto stagingBuffer = StagingBuffer::Builder()
        .alloc(texture.getSyncDataSize())
        .build(*_deviceAllocator, &_memoryPool);
    stagingBuffer->setData(texture.getSyncData());

    const auto releaseCommand = beginOneTimeTransfer(_transferCommandPool);
    texture.recordImageTransition(releaseCommand, vk::ImageLayout::eTransferDstOptimal);
    texture.recordBufferTransfer(releaseCommand, stagingBuffer->getVulkanBuffer());

    // Signals the fence after all queue jobs finish, allowing resource cleanup in the deletion worker's thread
    const auto deletionFence = _device.createFence(vk::FenceCreateInfo{});

    if (_transferFamilyIndex != _graphicsFamilyIndex) {
        texture.recordOwnershipRelease(releaseCommand, _transferFamilyIndex, _graphicsFamilyIndex, TEXTURE_FINAL_LAYOUT);
        const auto ownershipSemaphoreInfo = createOwnershipSemaphoreInfo();
        endReleaseCommands(releaseCommand, ownershipSemaphoreInfo);

        const auto acquireCommand = beginOneTimeTransfer(_graphicsCommandPool);
        texture.recordOwnershipAcquire(acquireCommand, _transferFamilyIndex, _graphicsFamilyIndex, TEXTURE_FINAL_LAYOUT);
        endAcquireCommands(acquireCommand, ownershipSemaphoreInfo, deletionFence);

        _stagingDeletionQueue.submit(
            _device, ownershipSemaphoreInfo.semaphore, deletionFence, std::move(stagingBuffer),
            { { _graphicsCommandPool, acquireCommand }, { _transferCommandPool, releaseCommand } });

    } else {
        texture.recordImageTransition(releaseCommand, TEXTURE_FINAL_LAYOUT);
        endOneTimeTransfer(releaseCommand, deletionFence);
        _stagingDeletionQueue.submit(_device, {}, deletionFence, std::move(stagingBuffer), {{ _transferCommandPool, releaseCommand }});
    }
}

void tpd::Engine::syncAndGenMips(Texture& texture) {
    if (!texture.hasSyncData()) {
        PLOGW << "Engine - Syncing an empty Texture: did you forget to call Texture::setSyncData()?";
        return;
    }

    auto stagingBuffer = StagingBuffer::Builder()
        .alloc(texture.getSyncDataSize())
        .build(*_deviceAllocator, &_memoryPool);
    stagingBuffer->setData(texture.getSyncData());

    const auto releaseCommand = beginOneTimeTransfer(_transferCommandPool);
    texture.recordBufferTransfer(releaseCommand, stagingBuffer->getVulkanBuffer());

    // Signals the fence after all queue jobs finish, allowing resource cleanup in the deletion worker's thread
    const auto deletionFence = _device.createFence(vk::FenceCreateInfo{});

    if (_transferFamilyIndex != _graphicsFamilyIndex) {
        texture.recordOwnershipRelease(releaseCommand, _transferFamilyIndex, _graphicsFamilyIndex);
        const auto ownershipSemaphoreInfo = createOwnershipSemaphoreInfo();
        endReleaseCommands(releaseCommand, ownershipSemaphoreInfo);

        const auto acquireCommand = beginOneTimeTransfer(_graphicsCommandPool);
        texture.recordOwnershipAcquire(acquireCommand, _transferFamilyIndex, _graphicsFamilyIndex);
        texture.recordMipsGeneration(releaseCommand, _physicalDevice);  // image is now in shader read optimal layout
        endAcquireCommands(acquireCommand, ownershipSemaphoreInfo, deletionFence);

        _stagingDeletionQueue.submit(
            _device, ownershipSemaphoreInfo.semaphore, deletionFence, std::move(stagingBuffer),
            { { _graphicsCommandPool, acquireCommand }, { _transferCommandPool, releaseCommand } });

    } else {
        texture.recordMipsGeneration(releaseCommand, _physicalDevice);  // image is now in shader read optimal layout
        endOneTimeTransfer(releaseCommand, deletionFence);
        _stagingDeletionQueue.submit(_device, {}, deletionFence, std::move(stagingBuffer), {{ _transferCommandPool, releaseCommand }});
    }
}

void tpd::Engine::destroy() noexcept {
    if (_initialized) {
        _initialized = false;

        _stagingDeletionQueue.waitEmpty();
        _deviceAllocator->destroy();

        _drawingCommandBuffers.clear();
        _device.destroyCommandPool(_drawingCommandPool);

        _device.destroyCommandPool(_transferCommandPool);
        _device.destroyCommandPool(_graphicsCommandPool);

        // It's possible that the Context has bound to another Engine, in which case resetting the renderer
        // would destroy the incorrect resources. We check the pointer state to tell if this has happened.
        if (_renderer) {
            _renderer->resetEngine();
            _renderer = nullptr;
        }

        _device.destroy();
        _device = nullptr;
        _physicalDevice = nullptr;
    }
}

tpd::Engine::~Engine() noexcept {
    Engine::destroy();
}
