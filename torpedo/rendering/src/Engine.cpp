#include "torpedo/rendering/Engine.h"
#include "torpedo/rendering/Renderer.h"

#include <torpedo/bootstrap/PhysicalDeviceSelector.h>
#include <torpedo/bootstrap/DebugUtils.h>

#include <plog/Log.h>

#include <format>

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
    _device = createDevice(extensions, { selection.graphicsQueueFamilyIndex, selection.transferQueueFamilyIndex,
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

    // Create drawing resources
    createDrawingCommandPool(selection.graphicsQueueFamilyIndex);
    createDrawingCommandBuffers();

    // This means a physical and logical device have been selected,
    // and the associated Renderer has obtained the those devices
    _initialized = true;
}

void tpd::Engine::createDrawingCommandPool(const uint32_t graphicsFamilyIndex) {
    auto poolInfo = vk::CommandPoolCreateInfo{};
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    poolInfo.queueFamilyIndex = graphicsFamilyIndex;
    _drawingCommandPool = _device.createCommandPool(poolInfo);
}

void tpd::Engine::createDrawingCommandBuffers() {
    auto allocInfo = vk::CommandBufferAllocateInfo{};
    allocInfo.commandPool = _drawingCommandPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = _renderer->getInFlightFramesCount();
    _drawingCommandBuffers = _device.allocateCommandBuffers(allocInfo);
}

void tpd::Engine::transitionImageLayout(
    const vk::CommandBuffer buffer,
    const vk::Image image,
    const vk::ImageLayout oldLayout,
    const vk::ImageLayout newLayout)
{
    auto barrier = vk::ImageMemoryBarrier2{};
    barrier.image = image;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;

    using enum vk::ImageAspectFlagBits;
    const auto aspectMask = newLayout == vk::ImageLayout::eDepthAttachmentOptimal ? eDepth : eColor;
    barrier.subresourceRange = {
        aspectMask, /* base mip */ 0, vk::RemainingMipLevels, /* base array layer */ 0, vk::RemainingArrayLayers };

    // TODO: define optimal masks for different use cases
    barrier.srcStageMask  = vk::PipelineStageFlagBits2::eAllCommands;
    barrier.srcAccessMask = vk::AccessFlagBits2::eMemoryWrite;
    barrier.dstStageMask  = vk::PipelineStageFlagBits2::eAllCommands;
    barrier.dstAccessMask = vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite;

    auto dependency = vk::DependencyInfo{};
    dependency.imageMemoryBarrierCount = 1;
    dependency.pImageMemoryBarriers = &barrier;

    buffer.pipelineBarrier2(dependency);
}

void tpd::Engine::logExtensions(
    const std::string_view extensionType,
    const std::string_view className,
    const std::vector<const char*>& extensions)
{
    const auto terminateString = extensions.empty() ? " (none)" : std::format(" ({}):", extensions.size());
    PLOGD << extensionType << " extensions required by " << className << terminateString;
    for (const auto& extension : extensions) {
        PLOGD << " - " << extension;
    }
}

void tpd::Engine::destroy() noexcept {
    if (_initialized) {
        _initialized = false;

        _drawingCommandBuffers.clear();
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
