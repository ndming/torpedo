#include "torpedo/rendering/Engine.h"
#include "torpedo/rendering/Renderer.h"
#include "torpedo/rendering/Utils.h"

#include <torpedo/bootstrap/DebugUtils.h>
#include <torpedo/foundation/StagingBuffer.h>

#include <plog/Log.h>

void tpd::Engine::init(Renderer& renderer) {
    // It's possible this engine has already been initialized,
    // in which case we must destroy all previously created resources
    destroy();

    auto rendererDeviceExtensions = renderer.getDeviceExtensions();
    auto extensions = getDeviceExtensions();
    extensions.insert(extensions.end(),
        std::make_move_iterator(rendererDeviceExtensions.begin()),
        std::make_move_iterator(rendererDeviceExtensions.end()));

    // Init physical device and create the logical device
    const auto selection = pickPhysicalDevice(extensions, renderer.getVulkanInstance(), renderer.getVulkanSurface());
    _physicalDevice = selection.physicalDevice;
    _device = createDevice(extensions, {
        selection.graphicsQueueFamilyIndex, selection.transferQueueFamilyIndex,
        selection.computeQueueFamilyIndex, selection.presentQueueFamilyIndex });

    PLOGI << "Found a suitable device for " << getName() << ": " << _physicalDevice.getProperties().deviceName.data();

    // Init all relevant queues
    _graphicsQueue = _device.getQueue(selection.graphicsQueueFamilyIndex, 0);
    _transferQueue = _device.getQueue(selection.transferQueueFamilyIndex, 0);
    _computeQueue  = _device.getQueue(selection.computeQueueFamilyIndex, 0);

#ifndef NDEBUG
    bootstrap::setVulkanObjectName(
        static_cast<VkPhysicalDevice>(_physicalDevice), vk::ObjectType::ePhysicalDevice,
        getName() + std::string{ " - PhysicalDevice" },
        renderer.getVulkanInstance(), _device);

    bootstrap::setVulkanObjectName(
        static_cast<VkDevice>(_device), vk::ObjectType::eDevice,
        getName() + std::string{ " - Device" },
        renderer.getVulkanInstance(), _device);

    bootstrap::setVulkanObjectName(
        static_cast<VkQueue>(_graphicsQueue), vk::ObjectType::eQueue,
        getName() + std::string{ " - GraphicsQueue" },
        renderer.getVulkanInstance(), _device);

    bootstrap::setVulkanObjectName(
        static_cast<VkQueue>(_transferQueue), vk::ObjectType::eQueue,
        getName() + std::string{ " - TransferQueue" },
        renderer.getVulkanInstance(), _device);

    bootstrap::setVulkanObjectName(
        static_cast<VkQueue>(_computeQueue), vk::ObjectType::eQueue,
        getName() + std::string{ " - ComputeQueue" },
        renderer.getVulkanInstance(), _device);
#endif

    // Pass the selected devices to associated renderer
    renderer.engineInit(_device, selection);
    // When destroying the engine, we will also need to clear the Vulkan objects init to the renderer
    _renderer = &renderer;

    // This means a physical and logical device have been selected,
    // and the associated Renderer has obtained the those devices
    _initialized = true;

    // Create common drawing and transfer resources
    createDrawingCommandPool(selection.graphicsQueueFamilyIndex);
    createStartupCommandPool(selection.transferQueueFamilyIndex);
    createDrawingCommandBuffers();

    PLOGD << "No. of drawing command buffers created: " << _drawingCommandBuffers.size();
    PLOGD << "No. of in-flight frames from renderer:  " << renderer.getInFlightFramesCount();

    // Create a device allocator
    _deviceAllocator = DeviceAllocator::Builder()
        .flags(VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT | VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT)
        .vulkanApiVersion(VK_API_VERSION_1_3)
        .build(renderer.getVulkanInstance(), _physicalDevice, _device, &_engineObjectPool);

    PLOGI << "Using VMA API version: 1.3.0";
    PLOGD << "VMA created with the following flags (2):";
    PLOGD << " - VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT";
    PLOGD << " - VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT";

    // This means all Vulkan and Engine resources have been initialized,
    // and the associated Renderer has obtained relevant Vulkan objects
    _initialized = true;
}

std::vector<const char*> tpd::Engine::getDeviceExtensions() const {
    auto extensions = std::vector{
        vk::EXTMemoryBudgetExtensionName,    // help VMA estimate memory budget more accurately
        vk::EXTMemoryPriorityExtensionName,  // incorporate memory priority to the allocator
    };
    rendering::logExtensions("Device", Engine::getName(), extensions);
    return extensions;
}

void tpd::Engine::createDrawingCommandPool(const uint32_t graphicsFamilyIndex) {
    // This pool allocates command buffers for drawing commands
    const auto poolInfo = vk::CommandPoolCreateInfo{}
        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
        .setQueueFamilyIndex(graphicsFamilyIndex);
    _drawingCommandPool = _device.createCommandPool(poolInfo);
}

void tpd::Engine::createStartupCommandPool(const uint32_t transferFamilyIndex) {
    // This pool allocates command buffers for one-shot transfers at the start of the program
    const auto poolInfo = vk::CommandPoolCreateInfo{}
        .setFlags(vk::CommandPoolCreateFlagBits::eTransient)
        .setQueueFamilyIndex(transferFamilyIndex);
    _startupCommandPool = _device.createCommandPool(poolInfo);
}

void tpd::Engine::createDrawingCommandBuffers() {
    auto allocInfo = vk::CommandBufferAllocateInfo{};
    allocInfo.commandPool = _drawingCommandPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = _renderer->getInFlightFramesCount();
    _drawingCommandBuffers = _device.allocateCommandBuffers(allocInfo);
}

vk::CommandBuffer tpd::Engine::beginOneTimeTransfer() const {
    const auto allocInfo = vk::CommandBufferAllocateInfo{}
        .setCommandPool(_startupCommandPool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(1);
    const auto buffer = _device.allocateCommandBuffers(allocInfo)[0];

    constexpr auto beginInfo = vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
    buffer.begin(beginInfo);
    return buffer;
}

void tpd::Engine::endOneTimeTransfer(const vk::CommandBuffer buffer, const bool wait) const {
    buffer.end();
    _transferQueue.submit(vk::SubmitInfo{ {}, {}, {}, 1, &buffer });
    if (wait) [[likely]] _transferQueue.waitIdle();
    _device.freeCommandBuffers(_startupCommandPool, 1, &buffer);
}

void tpd::Engine::sync(const StorageBuffer& storageBuffer) const {
    if (!storageBuffer.hasSyncData()) {
        PLOGW << "Engine - Syncing an empty StorageBuffer: did you forget to call StorageBuffer::setSyncData()?";
        return;
    }

    auto stagingBuffer = StagingBuffer::Builder()
        .alloc(storageBuffer.getSyncDataSize())
        .build(*_deviceAllocator);
    stagingBuffer.setData(storageBuffer.getSyncData());

    const auto cmd = beginOneTimeTransfer();
    storageBuffer.recordBufferTransfer(cmd, stagingBuffer.getVulkanBuffer());

    endOneTimeTransfer(cmd);
    stagingBuffer.destroy();
}

void tpd::Engine::sync(Texture& texture, const vk::ImageLayout finalLayout) const {
    if (!texture.hasSyncData()) {
        PLOGW << "Engine - Syncing an empty Texture: did you forget to call Texture::setSyncData()?";
        return;
    }

    const auto cmd = beginOneTimeTransfer();
    texture.recordImageTransition(cmd, vk::ImageLayout::eTransferDstOptimal);

    auto stagingBuffer = StagingBuffer::Builder()
        .alloc(texture.getSyncDataSize())
        .build(*_deviceAllocator);
    stagingBuffer.setData(texture.getSyncData());

    texture.recordBufferTransfer(cmd, stagingBuffer.getVulkanBuffer());
    texture.recordImageTransition(cmd, finalLayout);

    endOneTimeTransfer(cmd);
    stagingBuffer.destroy();
}

void tpd::Engine::syncAndGenMips(Texture& texture) const {
    if (!texture.hasSyncData()) {
        PLOGW << "Engine - Syncing an empty Texture: did you forget to call Texture::setSyncData()?";
        return;
    }

    const auto cmd = beginOneTimeTransfer();
    texture.recordImageTransition(cmd, vk::ImageLayout::eTransferDstOptimal);

    auto stagingBuffer = StagingBuffer::Builder()
        .alloc(texture.getSyncDataSize())
        .build(*_deviceAllocator);
    stagingBuffer.setData(texture.getSyncData());

    texture.recordBufferTransfer(cmd, stagingBuffer.getVulkanBuffer());
    texture.recordMipsGeneration(cmd, _physicalDevice);  // image is now in shader read optimal layout

    // Manually end and submit the operation with graphics queue since
    // blitting can only be performed on queues with graphics bit
    cmd.end();
    _graphicsQueue.submit(vk::SubmitInfo{ {}, {}, {}, 1, &cmd });
    _graphicsQueue.waitIdle();
    _device.freeCommandBuffers(_startupCommandPool, 1, &cmd);

    stagingBuffer.destroy();
}

void tpd::Engine::destroy() noexcept {
    if (_initialized) {
        _initialized = false;

        _deviceAllocator->destroy();

        _drawingCommandBuffers.clear();
        _device.destroyCommandPool(_startupCommandPool);
        _device.destroyCommandPool(_drawingCommandPool);

        _renderer->resetEngine();
        _renderer = nullptr;

        _device.destroy();
        _device = nullptr;
        _physicalDevice = nullptr;
    }
}

tpd::Engine::~Engine() noexcept {
    Engine::destroy();
}
