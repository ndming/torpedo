#include "torpedo/graphics/Renderer.h"

#include <torpedo/bootstrap/DeviceBuilder.h>

void tpd::Renderer::onCreate(const vk::Instance instance, const std::initializer_list<const char*> deviceExtensions) {
    onFeaturesRegister();
    pickPhysicalDevice(instance, deviceExtensions);
    createDevice(deviceExtensions);
    clearRendererFeatures();
}

void tpd::Renderer::onInitialize() {
    createDrawingCommandPool();
}

void tpd::Renderer::pickPhysicalDevice(const vk::Instance instance, const std::initializer_list<const char*> deviceExtensions) {
    const auto selector = getPhysicalDeviceSelector(deviceExtensions).select(instance);
    _physicalDevice = selector.getPhysicalDevice();
    _graphicsQueueFamily = selector.getGraphicsQueueFamily();
}

tpd::PhysicalDeviceSelector tpd::Renderer::getPhysicalDeviceSelector(const std::initializer_list<const char*> deviceExtensions) const {
    return PhysicalDeviceSelector()
        .deviceExtensions(getDeviceExtensions(deviceExtensions))
        .requestGraphicsQueueFamily()
        .features(getFeatures())
        .featuresVertexInputDynamicState(getVertexInputDynamicStateFeatures())
        .featuresExtendedDynamicState3(getExtendedDynamicState3Features());
}

void tpd::Renderer::createDevice(const std::initializer_list<const char*> deviceExtensions) {
    _device = DeviceBuilder()
        .deviceExtensions(getDeviceExtensions(deviceExtensions))
        .deviceFeatures(buildDeviceFeatures())
        .queueFamilyIndices({ _graphicsQueueFamily })
        .build(_physicalDevice);

    _graphicsQueue = _device.getQueue(_graphicsQueueFamily, 0);
}

std::vector<const char*> tpd::Renderer::getDeviceExtensions(const std::initializer_list<const char*> deviceExtensions) const {
    auto extensions = getRendererExtensions();
    extensions.insert(extensions.end(), deviceExtensions);
    return extensions;
}

std::vector<const char*> tpd::Renderer::getRendererExtensions() const {
    return {
        vk::EXTVertexInputDynamicStateExtensionName,  // dynamic vertex bindings and attributes
        vk::EXTExtendedDynamicState3ExtensionName,    // dynamic polygon mode and rasterization samples
    };
}

void tpd::Renderer::onFeaturesRegister() {
    const auto vertexInputDynamicStateFeatures = new vk::PhysicalDeviceVertexInputDynamicStateFeaturesEXT{ getVertexInputDynamicStateFeatures() };
    addFeature(
        vertexInputDynamicStateFeatures,
        &vertexInputDynamicStateFeatures->pNext,
        [](void* it) { delete static_cast<vk::PhysicalDeviceVertexInputDynamicStateFeaturesEXT*>(it); });

    const auto extendedDynamicState3Features = new vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT{ getExtendedDynamicState3Features() };
    addFeature(
        extendedDynamicState3Features,
        &extendedDynamicState3Features->pNext,
        [](void* it) { delete static_cast<vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT*>(it); });
}

vk::PhysicalDeviceFeatures tpd::Renderer::getFeatures() {
    auto features = vk::PhysicalDeviceFeatures{};
    features.largePoints       = vk::True;  // for gl_PointSize in vertex shader
    features.fillModeNonSolid  = vk::True;  // for custom polygon mode
    features.samplerAnisotropy = vk::True;  // required by raster materials
    features.sampleRateShading = vk::True;  // required by raster materials
    return features;
}

vk::PhysicalDeviceVertexInputDynamicStateFeaturesEXT tpd::Renderer::getVertexInputDynamicStateFeatures() {
    auto vertexInputDynamicStateFeatures = vk::PhysicalDeviceVertexInputDynamicStateFeaturesEXT{};
    vertexInputDynamicStateFeatures.vertexInputDynamicState = vk::True;
    return vertexInputDynamicStateFeatures;
}

vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT tpd::Renderer::getExtendedDynamicState3Features() {
    auto extendedDynamicState3Features = vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT{};
    extendedDynamicState3Features.extendedDynamicState3PolygonMode          = vk::True;
    extendedDynamicState3Features.extendedDynamicState3RasterizationSamples = vk::True;
    return extendedDynamicState3Features;
}

void tpd::Renderer::addFeature(void* const feature, void** const next, std::function<void(void*)>&& deallocate) {
    _rendererFeatures.emplace_back(feature, next, std::move(deallocate));
}

vk::PhysicalDeviceFeatures2 tpd::Renderer::buildDeviceFeatures() const {
    auto deviceFeatures = vk::PhysicalDeviceFeatures2{};
    deviceFeatures.features = getFeatures();

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
