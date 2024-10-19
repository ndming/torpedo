#include "torpedo/graphics/Renderer.h"

#include <torpedo/bootstrap/DeviceBuilder.h>

void tpd::Renderer::onCreate(const vk::Instance instance, const std::initializer_list<const char*> engineExtensions) {
    onFeaturesRegister();
    pickPhysicalDevice(instance, engineExtensions);
    createDevice(engineExtensions);
    clearRendererFeatures();
}

void tpd::Renderer::pickPhysicalDevice(const vk::Instance instance, const std::initializer_list<const char*> engineExtensions) {
    const auto selector = getPhysicalDeviceSelector(engineExtensions).select(instance);
    _physicalDevice = selector.getPhysicalDevice();
    _graphicsQueueFamily = selector.getGraphicsQueueFamily();
}

tpd::PhysicalDeviceSelector tpd::Renderer::getPhysicalDeviceSelector(const std::initializer_list<const char*> engineExtensions) const {
    return PhysicalDeviceSelector()
        .deviceExtensions(getDeviceExtensions(engineExtensions))
        .requestGraphicsQueueFamily();
}

void tpd::Renderer::createDevice(const std::initializer_list<const char*> engineExtensions) {
    _device = DeviceBuilder()
        .deviceExtensions(getDeviceExtensions(engineExtensions))
        .queueFamilyIndices({ _graphicsQueueFamily })
        .build(_physicalDevice);

    _graphicsQueue = _device.getQueue(_graphicsQueueFamily, 0);
}

std::vector<const char*> tpd::Renderer::getDeviceExtensions(const std::initializer_list<const char*> engineExtensions) const {
    auto extensions = getRendererExtensions();
    extensions.insert(extensions.end(), engineExtensions);
    return extensions;
}

std::vector<const char*> tpd::Renderer::getRendererExtensions() const {
    return {};
}

void tpd::Renderer::onFeaturesRegister() {
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
    for (const auto& [feature, _, deallocate] : _rendererFeatures) {
        deallocate(feature);
    }
    _rendererFeatures.clear();
}

void tpd::Renderer::onInitialize() {
    createDrawingCommandPool();

    createSharedDescriptorSetLayout();
    createSharedObjectBuffers();
    writeSharedDescriptorSets();
}

void tpd::Renderer::createDrawingCommandPool() {
    auto poolInfo = vk::CommandPoolCreateInfo{};
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    poolInfo.queueFamilyIndex = _graphicsQueueFamily;
    _drawingCommandPool = _device.createCommandPool(poolInfo);
}

void tpd::Renderer::createSharedDescriptorSetLayout() {
    _sharedDescriptorSetLayout = getSharedDescriptorLayoutBuilder().build(_device);
    _sharedDescriptorSets = _sharedDescriptorSetLayout->createInstance(_device, MAX_FRAMES_IN_FLIGHT);
}

void tpd::Renderer::createSharedObjectBuffers() const {
    const auto alignment = _physicalDevice.getProperties().limits.minUniformBufferOffsetAlignment;

    auto cameraObjectBufferBuilder = Buffer::Builder()
        .bufferCount(MAX_FRAMES_IN_FLIGHT)
        .usage(vk::BufferUsageFlagBits::eUniformBuffer);

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        cameraObjectBufferBuilder.buffer(i, sizeof(Camera::CameraObject), alignment);
    }

    Camera::_cameraObjectBuffer = cameraObjectBufferBuilder.build(*_allocator, ResourceType::Persistent);
}

void tpd::Renderer::writeSharedDescriptorSets() const {
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        auto cameraBufferInfo = vk::DescriptorBufferInfo{};
        cameraBufferInfo.buffer = Camera::_cameraObjectBuffer->getBuffer();
        cameraBufferInfo.offset = Camera::_cameraObjectBuffer->getOffsets()[i];
        cameraBufferInfo.range  = sizeof(Camera::CameraObject);

        _sharedDescriptorSets->setDescriptors(i, 0, 0, vk::DescriptorType::eUniformBuffer, _device, { cameraBufferInfo });
    }
}

std::unique_ptr<tpd::View> tpd::Renderer::createView() const {
    const auto [width, height] = getFramebufferSize();
    const auto w = static_cast<float>(width);
    const auto h = static_cast<float>(height);
    const auto viewport = vk::Viewport{ 0.0f, 0.0f, w, h, 0.0f, 1.0f };
    const auto scissor  = vk::Rect2D{ vk::Offset2D{ 0, 0 }, vk::Extent2D{ width, height } };
    return std::make_unique<View>(viewport, scissor);
}

void tpd::Renderer::waitIdle() const noexcept {
    _device.waitIdle();
}

void tpd::Renderer::onDestroy(const vk::Instance instance) noexcept {
    Camera::_cameraObjectBuffer->dispose(*_allocator);
    _sharedDescriptorSets->dispose(_device);
    _sharedDescriptorSetLayout->dispose(_device);
    _device.destroyCommandPool(_drawingCommandPool);
    _allocator = nullptr;
}
