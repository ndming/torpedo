#include "torpedo/volumetric/GaussianEngine.h"

#include <torpedo/bootstrap/DeviceBuilder.h>
#include <torpedo/foundation/ImageUtils.h>

#include <filesystem>

tpd::Engine::DrawPackage tpd::GaussianEngine::draw(const vk::Image image) const {
    const auto currentFrame = _renderer->getCurrentDrawingFrame();
    const auto buffer = _drawingCommandBuffers[currentFrame];

    buffer.reset();
    buffer.begin(vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

    // TODO: transition to a more optimal layout...
    foundation::recordLayoutTransition(
        buffer, image, vk::ImageAspectFlagBits::eColor, 1,
        vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);

    constexpr auto clearValue = vk::ClearColorValue{ std::array{ 0.0f, 0.0f, 1.0f, 1.0f } };
    constexpr auto clearRange = vk::ImageSubresourceRange{
        vk::ImageAspectFlagBits::eColor, 0, vk::RemainingMipLevels, 0, vk::RemainingArrayLayers };

    // TODO: ... then update this line
    buffer.clearColorImage(image, vk::ImageLayout::eGeneral, clearValue, clearRange);

    // TODO: ... and this line too
    foundation::recordLayoutTransition(
        buffer, image, vk::ImageAspectFlagBits::eColor, 1,
        vk::ImageLayout::eGeneral, vk::ImageLayout::ePresentSrcKHR);

    buffer.end();
    return { buffer, vk::PipelineStageFlagBits2::eAllGraphics };
}

tpd::PhysicalDeviceSelection tpd::GaussianEngine::pickPhysicalDevice(
    const std::vector<const char*>& deviceExtensions,
    const vk::Instance instance,
    const vk::SurfaceKHR surface) const
{
    auto selector = PhysicalDeviceSelector()
        .requestComputeQueueFamily()
        .featuresVulkan13(getVulkan13Features());

    if (_renderer->hasSurfaceRenderingSupport()) {
        selector.requestGraphicsQueueFamily();
        selector.requestPresentQueueFamily(surface);
    }

    auto selection = selector.select(instance, deviceExtensions);

    // Modify the queue family indices to minimize queue ownership transfer operations
    if (_renderer->hasSurfaceRenderingSupport()) {
        // We found a distinct transfer queue, but since we're not going to use graphics much,
        // we can set transfer to graphics and avoid an additional ownership transfer to present 
        if (selection.graphicsQueueFamilyIndex == selection.presentQueueFamilyIndex &&
            selection.transferQueueFamilyIndex != selection.graphicsQueueFamilyIndex) {
            selection.transferQueueFamilyIndex = selection.graphicsQueueFamilyIndex;
        }
    }

    PLOGD << "Queue family indices selected:";
    PLOGD << " - Transfer: " << selection.transferQueueFamilyIndex;
    PLOGD << " - Compute:  " << selection.computeQueueFamilyIndex;
    if (_renderer->hasSurfaceRenderingSupport()) {
        PLOGD << " - Present:  " << selection.presentQueueFamilyIndex;
    }

    return selection;
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
    PLOGD << "Assets directories used by " << getName() << ':';
    PLOGD << " - " << std::filesystem::path(TORPEDO_VOLUMETRIC_ASSETS_DIR);
    PLOGD << " - " << std::filesystem::path(TORPEDO_VOLUMETRIC_ASSETS_DIR) / "shaders";

    createRenderTargets();
    createPipelineResources();
}

void tpd::GaussianEngine::createRenderTargets() {
    const auto targetBuilder = Target::Builder()
        .extent(_renderer->getFramebufferSize())
        .aspect(vk::ImageAspectFlagBits::eColor)
        .format(vk::Format::eR8G8B8A8Unorm)
        .usage(vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc);

    // Create a render target image for each in-flight frame
    const auto frameCount = _renderer->getInFlightFramesCount();
    _targets.reserve(frameCount);     // reserve only, so that we can move-construct later with push_back
    _targetViews.resize(frameCount);  // vk::ImageView can be copied

    for (auto i = 0; i < frameCount; ++i) {
        _targets.push_back(targetBuilder.build(*_deviceAllocator));
        _targetViews[i] = _targets[i].createImageView(vk::ImageViewType::e2D, _device);
    }

    PLOGD << "Number of target images created by " << getName() << ": " << _targets.size();
}

void tpd::GaussianEngine::createPipelineResources() {
    _shaderLayout = ShaderLayout::Builder(&_engineResourcePool)
        .descriptorSetCount(1)
        .descriptor(0, 0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute)
        .buildUnique(_device);

    _shaderInstance = _shaderLayout->createInstance(&_engineResourcePool, _device, _renderer->getInFlightFramesCount());
}

void tpd::GaussianEngine::destroy() noexcept {
    if (_initialized) {
        _shaderInstance->destroy(_device);
        _shaderInstance.reset();

        _shaderLayout->destroy(_device);
        _shaderLayout.reset();

        std::ranges::for_each(_targetViews, [this](const auto it) { _device.destroyImageView(it); });
        _targets.clear();
    }
    Engine::destroy();
}
