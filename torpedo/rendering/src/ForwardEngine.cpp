#include "torpedo/rendering/ForwardEngine.h"
#include "torpedo/rendering/Renderer.h"
#include "torpedo/rendering/Utils.h"

#include <torpedo/bootstrap/DeviceBuilder.h>
#include <torpedo/bootstrap/PhysicalDeviceSelector.h>
#include <torpedo/foundation/ImageUtils.h>

#include <plog/Log.h>

std::vector<const char*> tpd::ForwardEngine::getDeviceExtensions() const {
    auto parentExtensions = Engine::getDeviceExtensions();
    rendering::logExtensions("Device", "tpd::ForwardEngine");
    return parentExtensions;
}

tpd::PhysicalDeviceSelection tpd::ForwardEngine::pickPhysicalDevice(
    const std::vector<const char*>& deviceExtensions,
    const vk::Instance instance,
    const vk::SurfaceKHR surface) const
{
    auto selector = PhysicalDeviceSelector()
        .requestGraphicsQueueFamily()
        .requestComputeQueueFamily()
        .featuresVulkan13(getVulkan13Features())
        .featuresVulkan12(getVulkan12Features());

    if (static_cast<VkSurfaceKHR>(surface) != VK_NULL_HANDLE) {
        selector.requestPresentQueueFamily(surface);
    }

    const auto selection = selector.select(instance, deviceExtensions);

    PLOGD << "Queue family indices selected:";
    PLOGD << " - Graphics: " << selection.graphicsQueueFamilyIndex;
    PLOGD << " - Transfer: " << selection.transferQueueFamilyIndex;
    PLOGD << " - Compute:  " << selection.computeQueueFamilyIndex;
    PLOGD << " - Present:  " << selection.presentQueueFamilyIndex;

    return selection;
}

vk::Device tpd::ForwardEngine::createDevice(
    const std::vector<const char*>& deviceExtensions,
    const std::initializer_list<uint32_t> queueFamilyIndices) const
{
    auto deviceFeatures = vk::PhysicalDeviceFeatures2{};
    auto featuresVulkan13 = getVulkan13Features();
    auto featuresVulkan12 = getVulkan12Features();

    deviceFeatures.pNext = &featuresVulkan13;
    featuresVulkan13.pNext = &featuresVulkan12;

    // Remember to update the count number at the end of the first message should more features are added
    PLOGD << "Device features requested by tpd::ForwardEngine (2):";
    PLOGD << " - Vulkan13Features: dynamicRendering, synchronization2";
    PLOGD << " - Vulkan12Features: bufferDeviceAddress, descriptorIndexing";

    return  DeviceBuilder()
        .deviceFeatures(&deviceFeatures)
        .queueFamilyIndices(queueFamilyIndices)
        .build(_physicalDevice, deviceExtensions);
}

vk::PhysicalDeviceVulkan13Features tpd::ForwardEngine::getVulkan13Features() {
    auto features = vk::PhysicalDeviceVulkan13Features();
    features.dynamicRendering = true;
    features.synchronization2 = true;
    return features;
}

vk::PhysicalDeviceVulkan12Features tpd::ForwardEngine::getVulkan12Features() {
    auto features = vk::PhysicalDeviceVulkan12Features();
    features.bufferDeviceAddress = true;
    features.descriptorIndexing  = true;
    return features;
}

vk::CommandBuffer tpd::ForwardEngine::draw(const vk::Image image) const {
    const auto currentFrame = _renderer->getCurrentFrame();
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
    return buffer;
}

void tpd::ForwardEngine::destroy() noexcept {
    if (_initialized) {
        // Make sure the GPU has stopped doing its things
        _device.waitIdle();
    }
    Engine::destroy();
}

tpd::ForwardEngine::~ForwardEngine() noexcept {
    destroy();
}
