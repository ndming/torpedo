#include "torpedo/rendering/Engine.h"
#include "torpedo/rendering/LogUtils.h"

#include <torpedo/bootstrap/DebugUtils.h>
#include <torpedo/bootstrap/DeviceBuilder.h>
#include <torpedo/foundation/StagingBuffer.h>

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
    // These pools handle transferring resources between queues
    createSyncWorkCommandPools();

    // Drawing command buffers used by downstream
    createDrawingCommandPool();
    createDrawingCommandBuffers();
    PLOGD << "Number of drawing command buffers created by " << Engine::getName() << ": " << _drawingCommandBuffers.size();

    // Create a device allocator
    _deviceAllocator = DeviceAllocator::Builder()
        .flags(VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT | VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT)
        .vulkanApiVersion(VK_API_VERSION_1_3)
        .build(instance, _physicalDevice, _device, &_syncResourcePool);

    PLOGI << "Using VMA API version: 1.3";
    PLOGD << "VMA created with the following flags (2):";
    PLOGD << " - VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT";
    PLOGD << " - VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT";
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
        .requestGraphicsQueueFamily();

    if (_renderer->hasSurfaceRenderingSupport()) {
        selector.requestPresentQueueFamily(surface);
    }

    const auto selection = selector.select(instance, deviceExtensions);
    PLOGD << "Queue family indices selected:";
    PLOGD << " - Graphics: " << selection.graphicsQueueFamilyIndex;
    PLOGD << " - Transfer: " << selection.transferQueueFamilyIndex;
    if (_renderer->hasSurfaceRenderingSupport()) {
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

void tpd::Engine::createSyncWorkCommandPools() {
    // These pools allocate command buffers for sync transfers at the start of the program
    _transferPool = _device.createCommandPool({ vk::CommandPoolCreateFlagBits::eTransient, _transferFamilyIndex });
    _computePool  = _device.createCommandPool({ vk::CommandPoolCreateFlagBits::eTransient, _computeFamilyIndex  });
    // Unlike transfer and compute, an Engine implementation may not request a graphics family, which renders
    // the _graphicsFamilyIndex invalid. However, it's very unlikely to cause any issue in this case since the
    // pool should be ignored by the implementation.
    _graphicsPool = _device.createCommandPool({ vk::CommandPoolCreateFlagBits::eTransient, _graphicsFamilyIndex });
}

void tpd::Engine::createDrawingCommandPool() {
    // This pool allocates command buffers for drawing commands
    const auto poolInfo = vk::CommandPoolCreateInfo{}
        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
        .setQueueFamilyIndex(_graphicsFamilyIndex);
    _drawingCommandPool = _device.createCommandPool(poolInfo);
}

void tpd::Engine::createDrawingCommandBuffers() {
    auto allocInfo = vk::CommandBufferAllocateInfo{};
    allocInfo.commandPool = _drawingCommandPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = _renderer->getInFlightFramesCount();

    const auto allocatedBuffers = _device.allocateCommandBuffers(allocInfo);
    _drawingCommandBuffers.resize(allocatedBuffers.size());
    for (size_t i = 0; i < allocatedBuffers.size(); ++i) {
        _drawingCommandBuffers[i] = allocatedBuffers[i];
    }
}

vk::CommandPool tpd::Engine::getSyncPool(const uint32_t queueFamily) const {
    if (queueFamily == _transferFamilyIndex) {
        return _transferPool;
    }
    if (queueFamily == _computeFamilyIndex) {
        return _computePool;
    }
    if (queueFamily == _graphicsFamilyIndex) {
        return _graphicsPool;
    }
    throw std::invalid_argument("Engine - Unrecognized queue family for sync command pool");
}

vk::CommandBuffer tpd::Engine::beginSyncTransfer(const uint32_t queueFamily) {
    const auto allocInfo = vk::CommandBufferAllocateInfo{}
        .setCommandPool(getSyncPool(queueFamily))
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(1);

    // The command pool might be accessed from the deletion worker thread
    std::lock_guard lock(_syncWorkPoolMutex);
    const auto buffer = _device.allocateCommandBuffers(allocInfo)[0];

    constexpr auto beginInfo = vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
    buffer.begin(beginInfo);
    return buffer;
}

void tpd::Engine::endSyncCommands(const vk::CommandBuffer buffer, const vk::Fence deletionFence) const {
    buffer.end();
    _transferQueue.submit(vk::SubmitInfo{ {}, {}, {}, 1, &buffer }, deletionFence);
}

vk::SemaphoreSubmitInfo tpd::Engine::createSyncOwnershipSemaphoreInfo() const {
    const auto ownershipTransferSemaphore = _device.createSemaphore({});
    auto semaphoreInfo = vk::SemaphoreSubmitInfo{};
    semaphoreInfo.semaphore = ownershipTransferSemaphore;
    // The only valid wait stage for ownership transfer is all-command
    semaphoreInfo.stageMask = vk::PipelineStageFlagBits2::eAllCommands;
    semaphoreInfo.deviceIndex = 0;
    semaphoreInfo.value = 1;
    return semaphoreInfo;
}

void tpd::Engine::endSyncReleaseCommands(const vk::CommandBuffer buffer, const vk::SemaphoreSubmitInfo& semaphoreInfo) const {
    buffer.end();

    auto releaseInfo = vk::CommandBufferSubmitInfo{};
    releaseInfo.commandBuffer = buffer;
    releaseInfo.deviceMask = 0b1;  // ignored by single-GPU setups

    const auto releaseSubmitInfo = vk::SubmitInfo2{}
        .setCommandBufferInfos(releaseInfo)
        .setSignalSemaphoreInfos(semaphoreInfo);
    _transferQueue.submit2(releaseSubmitInfo);
}

void tpd::Engine::endSyncAcquireCommands(
    const vk::CommandBuffer buffer,
    const vk::SemaphoreSubmitInfo& semaphoreInfo,
    const uint32_t acquireFamily,
    const vk::Fence deletionFence) const
{
    buffer.end();

    auto acquireInfo = vk::CommandBufferSubmitInfo{};
    acquireInfo.commandBuffer = buffer;
    acquireInfo.deviceMask = 0b1;  // ignored by single-GPU setups

    const auto acquireSubmitInfo = vk::SubmitInfo2{}
        .setCommandBufferInfos(acquireInfo)
        .setWaitSemaphoreInfos(semaphoreInfo);

    if (acquireFamily == _graphicsFamilyIndex) {
        _graphicsQueue.submit2(acquireSubmitInfo, deletionFence);
    } else if (acquireFamily == _computeFamilyIndex) {
        _computeQueue.submit2(acquireSubmitInfo, deletionFence);
    } else {
        throw std::invalid_argument("Engine - Acquire queue family must be either compute or graphics family");
    }
}

void tpd::Engine::sync(const StorageBuffer& storageBuffer, const uint32_t acquireFamily) {
    if (!storageBuffer.hasSyncData()) {
        PLOGW << "Engine - Syncing an empty StorageBuffer: did you forget to call StorageBuffer::setSyncData()?";
        return;
    }

    auto stagingBuffer = StagingBuffer::Builder()
        .alloc(storageBuffer.getSyncDataSize())
        .build(*_deviceAllocator, &_syncResourcePool);
    stagingBuffer->setData(storageBuffer.getSyncData());

    const auto releaseCommand = beginSyncTransfer(_transferFamilyIndex);
    storageBuffer.recordBufferTransfer(releaseCommand, stagingBuffer->getVulkanBuffer());

    // Signals the fence after all queue jobs finish, allowing resource cleanup in the deletion worker's thread
    const auto deletionFence = _device.createFence(vk::FenceCreateInfo{});

    if (_transferFamilyIndex != acquireFamily) {
        storageBuffer.recordOwnershipRelease(releaseCommand, _transferFamilyIndex, acquireFamily);
        const auto ownershipSemaphoreInfo = createSyncOwnershipSemaphoreInfo();
        endSyncReleaseCommands(releaseCommand, ownershipSemaphoreInfo);

        const auto acquireCommand = beginSyncTransfer(acquireFamily);
        storageBuffer.recordOwnershipAcquire(acquireCommand, _transferFamilyIndex, acquireFamily);
        endSyncAcquireCommands(acquireCommand, ownershipSemaphoreInfo, acquireFamily, deletionFence);

        _stagingDeletionQueue.submit(
            _device, ownershipSemaphoreInfo.semaphore, deletionFence, std::move(stagingBuffer),
            { { getSyncPool(acquireFamily), acquireCommand }, { _transferPool, releaseCommand } });

    } else {
        // Ensure subsequent commands don't access the buffer during copy
        storageBuffer.recordTransferDstSync(releaseCommand);
        endSyncCommands(releaseCommand, deletionFence);
        _stagingDeletionQueue.submit(_device, {}, deletionFence, std::move(stagingBuffer), {{ _transferPool, releaseCommand }});
    }
}

void tpd::Engine::sync(Texture& texture, const uint32_t acquireFamily) {
    if (!texture.hasSyncData()) {
        PLOGW << "Engine - Syncing an empty Texture: did you forget to call Texture::setSyncData()?";
        return;
    }

    auto stagingBuffer = StagingBuffer::Builder()
        .alloc(texture.getSyncDataSize())
        .build(*_deviceAllocator, &_syncResourcePool);
    stagingBuffer->setData(texture.getSyncData());

    const auto releaseCommand = beginSyncTransfer(_transferFamilyIndex);
    texture.recordImageTransition(releaseCommand, vk::ImageLayout::eTransferDstOptimal);
    texture.recordBufferTransfer(releaseCommand, stagingBuffer->getVulkanBuffer());

    // Signals the fence after all queue jobs finish, allowing resource cleanup in the deletion worker's thread
    const auto deletionFence = _device.createFence(vk::FenceCreateInfo{});

    if (_transferFamilyIndex != acquireFamily) {
        texture.recordOwnershipRelease(releaseCommand, _transferFamilyIndex, acquireFamily, TEXTURE_FINAL_LAYOUT);
        const auto ownershipSemaphoreInfo = createSyncOwnershipSemaphoreInfo();
        endSyncReleaseCommands(releaseCommand, ownershipSemaphoreInfo);

        const auto acquireCommand = beginSyncTransfer(acquireFamily);
        texture.recordOwnershipAcquire(acquireCommand, _transferFamilyIndex, acquireFamily, TEXTURE_FINAL_LAYOUT);
        endSyncAcquireCommands(acquireCommand, ownershipSemaphoreInfo, acquireFamily, deletionFence);

        _stagingDeletionQueue.submit(
            _device, ownershipSemaphoreInfo.semaphore, deletionFence, std::move(stagingBuffer),
            { { getSyncPool(acquireFamily), acquireCommand }, { _transferPool, releaseCommand } });

    } else {
        texture.recordImageTransition(releaseCommand, TEXTURE_FINAL_LAYOUT);
        endSyncCommands(releaseCommand, deletionFence);
        _stagingDeletionQueue.submit(_device, {}, deletionFence, std::move(stagingBuffer), {{ _transferPool, releaseCommand }});
    }
}

void tpd::Engine::syncAndGenMips(Texture& texture, const uint32_t acquireFamily) {
    if (!texture.hasSyncData()) {
        PLOGW << "Engine - Syncing an empty Texture: did you forget to call Texture::setSyncData()?";
        return;
    }

    auto stagingBuffer = StagingBuffer::Builder()
        .alloc(texture.getSyncDataSize())
        .build(*_deviceAllocator, &_syncResourcePool);
    stagingBuffer->setData(texture.getSyncData());

    const auto releaseCommand = beginSyncTransfer(_transferFamilyIndex);
    texture.recordBufferTransfer(releaseCommand, stagingBuffer->getVulkanBuffer());

    // Signals the fence after all queue jobs finish, allowing resource cleanup in the deletion worker's thread
    const auto deletionFence = _device.createFence(vk::FenceCreateInfo{});

    if (_transferFamilyIndex != acquireFamily) {
        texture.recordOwnershipRelease(releaseCommand, _transferFamilyIndex, acquireFamily);
        const auto ownershipSemaphoreInfo = createSyncOwnershipSemaphoreInfo();
        endSyncReleaseCommands(releaseCommand, ownershipSemaphoreInfo);

        const auto acquireCommand = beginSyncTransfer(acquireFamily);
        texture.recordOwnershipAcquire(acquireCommand, _transferFamilyIndex, acquireFamily);
        texture.recordMipsGeneration(releaseCommand, _physicalDevice);  // image is now in shader read optimal layout
        endSyncAcquireCommands(acquireCommand, ownershipSemaphoreInfo, acquireFamily, deletionFence);

        _stagingDeletionQueue.submit(
            _device, ownershipSemaphoreInfo.semaphore, deletionFence, std::move(stagingBuffer),
            { { getSyncPool(acquireFamily), acquireCommand }, { _transferPool, releaseCommand } });

    } else {
        texture.recordMipsGeneration(releaseCommand, _physicalDevice);  // image is now in shader read optimal layout
        endSyncCommands(releaseCommand, deletionFence);
        _stagingDeletionQueue.submit(_device, {}, deletionFence, std::move(stagingBuffer), {{ _transferPool, releaseCommand }});
    }
}

void tpd::Engine::destroy() noexcept {
    if (_initialized) {
        _initialized = false;

        _stagingDeletionQueue.waitEmpty();
        _deviceAllocator->destroy();

        _drawingCommandBuffers.clear();
        _device.destroyCommandPool(_drawingCommandPool);

        _device.destroyCommandPool(_graphicsPool);
        _device.destroyCommandPool(_computePool);
        _device.destroyCommandPool(_transferPool);

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
