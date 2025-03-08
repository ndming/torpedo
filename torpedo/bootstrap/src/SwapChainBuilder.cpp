#include "torpedo/bootstrap/SwapChainBuilder.h"

tpd::SwapChain tpd::SwapChainBuilder::build(
    const vk::SurfaceKHR surface,
    const vk::PhysicalDevice physicalDevice,
    const vk::Device device) const
{
    auto swapChain = SwapChain{};

    swapChain.surfaceFormat = chooseSurfaceFormat(physicalDevice.getSurfaceFormatsKHR(surface));
    swapChain.presentMode = choosePresentMode(physicalDevice.getSurfacePresentModesKHR(surface));
    swapChain.extent = chooseExtent(physicalDevice.getSurfaceCapabilitiesKHR(surface));

    const auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    swapChain.minImageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && swapChain.minImageCount > capabilities.maxImageCount) [[unlikely]] {
        swapChain.minImageCount = capabilities.maxImageCount;
    }

    auto createInfo = vk::SwapchainCreateInfoKHR{};
    createInfo.surface = surface;
    createInfo.minImageCount = swapChain.minImageCount;
    createInfo.imageFormat = swapChain.surfaceFormat.format;
    createInfo.imageColorSpace = swapChain.surfaceFormat.colorSpace;
    createInfo.imageExtent = swapChain.extent;
    createInfo.presentMode = swapChain.presentMode;
    createInfo.imageUsage = _imageUsage;
    createInfo.imageArrayLayers = 1;  // will always be 1 unless for a stereoscopic 3D application
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.clipped = vk::True;
    createInfo.oldSwapchain = nullptr;

    if (_graphicsFamilyIndex != _presentFamilyIndex) [[unlikely]] {
        const uint32_t queueFamilyIndices[]{ _graphicsFamilyIndex, _presentFamilyIndex };
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    swapChain.swapChain = device.createSwapchainKHR(createInfo);
    return swapChain;
}

vk::SurfaceFormatKHR tpd::SwapChainBuilder::chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats) const {
    const auto suitable = [this](const auto& it){ return it.format == _desiredFormat && it.colorSpace == _desiredColorSpace; };
    if (const auto found = std::ranges::find_if(formats, suitable); found != formats.end()) {
        return *found;
    }
    return formats[0];
}

vk::PresentModeKHR tpd::SwapChainBuilder::choosePresentMode(const std::vector<vk::PresentModeKHR>& presentModes) const {
    if (const auto found = std::ranges::find(presentModes, _desiredPresentMode); found != presentModes.end()) {
        return _desiredPresentMode;
    }
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D tpd::SwapChainBuilder::chooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    auto actualExtent = _desiredExtent;
    actualExtent.width  = std::clamp(_desiredExtent.width,  capabilities.minImageExtent.width,  capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(_desiredExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}
