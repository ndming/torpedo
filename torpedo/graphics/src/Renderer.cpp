#include "torpedo/graphics/Renderer.h"

#include <torpedo/bootstrap/DeviceBuilder.h>

void tpd::Renderer::onCreate(const vk::Instance instance, const std::initializer_list<const char*> engineExtensions) {
    onFeaturesRegister();
    pickPhysicalDevice(instance, engineExtensions);
    createDevice(engineExtensions);
    clearRendererFeatures();
}

void tpd::Renderer::onInitialize() {
    createDrawingCommandPool();
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

void tpd::Renderer::createDrawingCommandPool() {
    auto poolInfo = vk::CommandPoolCreateInfo{};
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    poolInfo.queueFamilyIndex = _graphicsQueueFamily;
    _drawingCommandPool = _device.createCommandPool(poolInfo);
}

void tpd::Renderer::waitIdle() const noexcept {
    _device.waitIdle();
}

void tpd::Renderer::onDestroy(const vk::Instance instance) noexcept {
    _device.destroyCommandPool(_drawingCommandPool);
    _allocator = nullptr;
}
