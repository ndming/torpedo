#include "torpedo/volumetric/GaussianEngine.h"

#include <torpedo/bootstrap/DeviceBuilder.h>

#include <filesystem>

tpd::Engine::DrawPackage tpd::GaussianEngine::draw(const vk::Image image) const {
    return {};
}

tpd::PhysicalDeviceSelection tpd::GaussianEngine::pickPhysicalDevice(
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
    PLOGD << " - Transfer: " << selection.transferQueueFamilyIndex;
    PLOGD << " - Compute:  " << selection.computeQueueFamilyIndex;
    if (requestingPresentFamily) {
        PLOGD << " - Present:  " << selection.presentQueueFamilyIndex;
    }

    return selection;
}

vk::Device tpd::GaussianEngine::createDevice(
    const std::vector<const char*>& deviceExtensions,
    const std::initializer_list<uint32_t> queueFamilyIndices) const
{
    return DeviceBuilder()
        .queueFamilyIndices(queueFamilyIndices)
        .build(_physicalDevice, deviceExtensions);
}

void tpd::GaussianEngine::onInitialized() {
    PLOGD << "Assets directories used by tpd::GaussianEngine:";
    PLOGD << " - " << std::filesystem::path(TORPEDO_VOLUMETRIC_ASSETS_DIR);
    PLOGD << " - " << std::filesystem::path(TORPEDO_VOLUMETRIC_ASSETS_DIR) / "shaders";
}
