#include "torpedo/graphics/Renderer.h"
#include "torpedo/graphics/Material.h"

#include <torpedo/bootstrap/DeviceBuilder.h>
#include <torpedo/bootstrap/InstanceBuilder.h>
#include <torpedo/bootstrap/SamplerBuilder.h>

#include <plog/Log.h>

tpd::Renderer::Renderer(std::vector<const char*>&& requiredExtensions) {
    createInstance(std::move(requiredExtensions));
#ifndef NDEBUG
    createDebugMessenger();
#endif
    PLOGD << "Assets directory: " << TORPEDO_ASSETS_DIR;
}

#ifndef NDEBUG
// =====================================================================================================================
// DEBUG MESSENGER
// =====================================================================================================================

VKAPI_ATTR vk::Bool32 VKAPI_CALL torpedoDebugCallback(
    const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    [[maybe_unused]] const VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    [[maybe_unused]] void* const userData
) {
    switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            PLOGV << pCallbackData->pMessage; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            PLOGI << pCallbackData->pMessage; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            PLOGW << pCallbackData->pMessage; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            PLOGE << pCallbackData->pMessage; break;
        default: break;
    }

    return vk::False;
}

void tpd::Renderer::createDebugMessenger() {
    const auto debugInfo = bootstrap::createDebugInfo(torpedoDebugCallback);

    if (bootstrap::createDebugUtilsMessenger(_instance, &debugInfo, &_debugMessenger) == vk::Result::eErrorExtensionNotPresent) {
        throw std::runtime_error("Renderer - Failed to set up a debug messenger");
    }
}
#endif

void tpd::Renderer::createInstance(std::vector<const char*>&& requiredExtensions) {
    auto instanceCreateFlags = vk::InstanceCreateFlags{};

#ifdef __APPLE__
    // Beginning with the 1.3.216 Vulkan SDK, the VK_KHR_PORTABILITY_subset extension is mandatory
    // for macOS with the latest MoltenVK SDK
    requiredExtensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
    instanceCreateFlags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif

    _instance = InstanceBuilder()
        .applicationInfo(bootstrap::createApplicationInfo("torpedo", vk::makeApiVersion(0, 1, 3, 0)))
#ifndef NDEBUG
        .debugInfo(bootstrap::createDebugInfo(torpedoDebugCallback))
        .layers({ "VK_LAYER_KHRONOS_validation" })
#endif
        .extensions(std::move(requiredExtensions))
        .build(instanceCreateFlags);
}

void tpd::Renderer::init() {
    registerFeatures();
    pickPhysicalDevice();
    createDevice();
    clearRendererFeatures();

    createResourceAllocator();

    createDrawingCommandPool();
    createOneShotCommandPool();

    createSharedDescriptorSetLayout();
    createSharedObjectBuffers();
    writeSharedDescriptorSets();

    createDefaultResources();
}

std::vector<const char*> tpd::Renderer::getDeviceExtensions() {
    return {
        vk::EXTMemoryBudgetExtensionName,    // help VMA estimate memory budget more accurately
        vk::EXTMemoryPriorityExtensionName,  // incorporate memory priority to the allocator
    };
}

void tpd::Renderer::registerFeatures() {
}

vk::PhysicalDeviceFeatures2 tpd::Renderer::buildDeviceFeatures(const vk::PhysicalDeviceFeatures& features) const {
    auto deviceFeatures = vk::PhysicalDeviceFeatures2{};
    deviceFeatures.features = features;

    void** pNext = &deviceFeatures.pNext;
    for (const auto& [feature, next, _] : _rendererFeatures) {
        *pNext = feature;
        pNext = next;
    }

    return deviceFeatures;
}

void tpd::Renderer::clearRendererFeatures() noexcept {
    for (const auto& [feature, _, deallocate] : _rendererFeatures) deallocate(feature);
    _rendererFeatures.clear();
}

void tpd::Renderer::createResourceAllocator() {
    _allocator = ResourceAllocator::Builder()
        .flags(VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT | VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT)
        .vulkanApiVersion(vk::makeApiVersion(0, 1, 3, 0))
        .build(_instance, _physicalDevice, _device);
}

void tpd::Renderer::createDrawingCommandPool() {
    auto poolInfo = vk::CommandPoolCreateInfo{};
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    poolInfo.queueFamilyIndex = _graphicsQueueFamily;
    _drawingCommandPool = _device.createCommandPool(poolInfo);
}

void tpd::Renderer::createOneShotCommandPool() {
    auto transferPoolInfo = vk::CommandPoolCreateInfo{};
    transferPoolInfo.flags = vk::CommandPoolCreateFlagBits::eTransient;
    transferPoolInfo.queueFamilyIndex = _graphicsQueueFamily;
    _oneShotCommandPool = _device.createCommandPool(transferPoolInfo);
}

void tpd::Renderer::createSharedDescriptorSetLayout() {
    _sharedShaderLayout = Material::getSharedLayoutBuilder().build(_device);
    _sharedShaderInstance = _sharedShaderLayout->createInstance(_device, MAX_FRAMES_IN_FLIGHT);
}

void tpd::Renderer::createSharedObjectBuffers() {
    auto cameraObjectBufferBuilder = Buffer::Builder()
        .bufferCount(MAX_FRAMES_IN_FLIGHT)
        .usage(vk::BufferUsageFlagBits::eUniformBuffer);

    auto lightObjectBufferBuilder = Buffer::Builder()
        .bufferCount(MAX_FRAMES_IN_FLIGHT)
        .usage(vk::BufferUsageFlagBits::eUniformBuffer);

    const auto alignment = getMinUniformBufferOffsetAlignment();
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        cameraObjectBufferBuilder.buffer(i, sizeof(Camera::CameraObject), alignment);
        lightObjectBufferBuilder.buffer(i, sizeof(Light::LightObject), alignment);
    }

    _cameraObjectBuffer = cameraObjectBufferBuilder.build(*_allocator, ResourceType::Persistent);
    _lightObjectBuffer = lightObjectBufferBuilder.build(*_allocator, ResourceType::Persistent);
}

void tpd::Renderer::writeSharedDescriptorSets() const {
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        auto cameraBufferInfo = vk::DescriptorBufferInfo{};
        cameraBufferInfo.buffer = _cameraObjectBuffer->getBuffer();
        cameraBufferInfo.offset = _cameraObjectBuffer->getOffsets()[i];
        cameraBufferInfo.range  = sizeof(Camera::CameraObject);

        auto lightBufferInfo = vk::DescriptorBufferInfo{};
        lightBufferInfo.buffer = _lightObjectBuffer->getBuffer();
        lightBufferInfo.offset = _lightObjectBuffer->getOffsets()[i];
        lightBufferInfo.range  = sizeof(Light::LightObject);

        _sharedShaderInstance->setDescriptors(i, 0, 0, vk::DescriptorType::eUniformBuffer, _device, { cameraBufferInfo });
        _sharedShaderInstance->setDescriptors(i, 0, 1, vk::DescriptorType::eUniformBuffer, _device, { lightBufferInfo });
    }
}

void tpd::Renderer::createDefaultResources() {
    _defaultShaderReadImage = Image::Builder()
        .dimensions(1, 1, 1)
        .type(vk::ImageType::e2D)
        .format(vk::Format::eR8G8B8A8Srgb)
        .usage(vk::ImageUsageFlagBits::eSampled)
        .imageViewType(vk::ImageViewType::e2D)
        .imageViewAspects(vk::ImageAspectFlagBits::eColor)
        .build(_device, *_allocator, ResourceType::Dedicated);

    using enum vk::ImageLayout;
    transitionImageLayout(eUndefined, eShaderReadOnlyOptimal, _defaultShaderReadImage->getImage());

    _defaultSampler = SamplerBuilder()
        .anisotropyEnabled(true)
        .maxAnisotropy(8.0f)
        .build(_device);
}

std::unique_ptr<tpd::View> tpd::Renderer::createView() const {
    const auto [width, height] = getFramebufferSize();
    const auto w = static_cast<float>(width);
    const auto h = static_cast<float>(height);
    const auto viewport = vk::Viewport{ 0.0f, 0.0f, w, h, 0.0f, 1.0f };
    const auto scissor  = vk::Rect2D{ vk::Offset2D{ 0, 0 }, vk::Extent2D{ width, height } };
    return std::make_unique<View>(viewport, scissor);
}

void tpd::Renderer::copyBufferToBuffer(const vk::Buffer src, const vk::Buffer dst, const vk::BufferCopy& copyInfo) const {
    const auto cmdBuffer = beginOneShotCommands();
    cmdBuffer.copyBuffer(src, dst, copyInfo);
    endOneShotCommands(cmdBuffer);
}

void tpd::Renderer::copyBufferToImage(const vk::Buffer buffer, const vk::Image image, const vk::BufferImageCopy& copyInfo) const {
    const auto cmdBuffer = beginOneShotCommands();
    cmdBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, copyInfo);
    endOneShotCommands(cmdBuffer);
}

void tpd::Renderer::transitionImageLayout(const vk::ImageLayout oldLayout, const vk::ImageLayout newLayout, const vk::Image image) const {
    const auto commandBuffer = beginOneShotCommands();

    auto barrier = vk::ImageMemoryBarrier{};
    barrier.image = image;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.subresourceRange = { vk::ImageAspectFlagBits::eColor, /* base mip */ 0, /* level count */ 1, 0, 1 };
    // We're not using the barrier for transferring queue family ownership.
    barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;

    vk::PipelineStageFlags srcStage;
    vk::PipelineStageFlags dstStage;

    using enum vk::ImageLayout;
    if (oldLayout == eUndefined && newLayout == eTransferDstOptimal) {
        // Transfer writes that don’t need to wait on anything, we specify an empty access mask and the earliest
        // possible pipeline stage: eTopOfPipe
        barrier.srcAccessMask = static_cast<vk::AccessFlags>(0);
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
        dstStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (oldLayout == eTransferDstOptimal && newLayout == eShaderReadOnlyOptimal) {
        // Shader reads should wait on transfer writes.
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        // The image will be written in the same pipeline stage and subsequently read by a shader
        srcStage = vk::PipelineStageFlagBits::eTransfer;
        dstStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else if (oldLayout == eUndefined && newLayout == eShaderReadOnlyOptimal) {
        // When transitioning from eUndefined to eShaderReadOnlyOptimal, no need to wait on anything.
        barrier.srcAccessMask = static_cast<vk::AccessFlags>(0);  // No previous data to wait on
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;   // Image will be read by shaders
        srcStage = vk::PipelineStageFlagBits::eTopOfPipe;          // Earliest stage since we're not waiting for anything
        dstStage = vk::PipelineStageFlagBits::eFragmentShader;     // Transitioning for shader use
    } else {
        throw std::invalid_argument("Renderer - Unsupported layout transition");
    }

    commandBuffer.pipelineBarrier(
        srcStage, dstStage,
        static_cast<vk::DependencyFlags>(0),
        {},      // for memory barriers
        {},      // for buffer memory barriers
        barrier  // for image memory barriers
    );
    endOneShotCommands(commandBuffer);
}

void tpd::Renderer::waitIdle() const noexcept {
    _device.waitIdle();
}

vk::CommandBuffer tpd::Renderer::beginOneShotCommands() const {
    const auto allocInfo = vk::CommandBufferAllocateInfo{ _oneShotCommandPool, vk::CommandBufferLevel::ePrimary, 1 };
    const auto commandBuffer = _device.allocateCommandBuffers(allocInfo)[0];
    constexpr auto beginInfo = vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
    commandBuffer.begin(beginInfo);
    return commandBuffer;
}

void tpd::Renderer::endOneShotCommands(const vk::CommandBuffer commandBuffer) const {
    commandBuffer.end();
    _graphicsQueue.submit(vk::SubmitInfo{ {}, {}, {}, 1, &commandBuffer });
    _graphicsQueue.waitIdle();
    _device.freeCommandBuffers(_oneShotCommandPool, 1, &commandBuffer);
}

tpd::Renderer::~Renderer() {
    _device.destroySampler(_defaultSampler);
    _defaultShaderReadImage->dispose(_device, *_allocator);
    _lightObjectBuffer->dispose(*_allocator);
    _cameraObjectBuffer->dispose(*_allocator);
    _sharedShaderInstance->dispose(_device);
    _sharedShaderLayout->dispose(_device);
    _device.destroyCommandPool(_drawingCommandPool);
    _device.destroyCommandPool(_oneShotCommandPool);
    _allocator.reset();
    _device.destroy();
#ifndef NDEBUG
    bootstrap::destroyDebugUtilsMessenger(_instance, _debugMessenger);
#endif
    _instance.destroy();
}
