#pragma once

#include <vulkan/vulkan.hpp>

namespace tpd {
    struct SwapChain {
        vk::SwapchainKHR swapChain{};
        vk::SurfaceFormatKHR surfaceFormat{};
        vk::PresentModeKHR presentMode{};
        vk::Extent2D extent{};
        uint32_t minImageCount{};
    };

    class SwapChainBuilder {
    public:
        SwapChainBuilder& desiredSurfaceFormat(vk::Format format, vk::ColorSpaceKHR colorSpace);
        SwapChainBuilder& desiredPresentMode(vk::PresentModeKHR presentMode);
        SwapChainBuilder& desiredExtent(uint32_t width, uint32_t height);

        SwapChainBuilder& imageUsageFlags(vk::ImageUsageFlags usage);
        SwapChainBuilder& queueFamilyIndices(uint32_t graphicsQueue, uint32_t presentQueue);

        [[nodiscard]] SwapChain build(vk::SurfaceKHR surface, vk::PhysicalDevice physicalDevice, vk::Device device) const;

    private:
        vk::SurfaceFormatKHR chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats) const;
        vk::PresentModeKHR choosePresentMode(const std::vector<vk::PresentModeKHR>& presentModes) const;
        vk::Extent2D chooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const;

        vk::Format _desiredFormat{ vk::Format::eUndefined };
        vk::ColorSpaceKHR _desiredColorSpace{ vk::ColorSpaceKHR::eSrgbNonlinear };
        vk::PresentModeKHR _desiredPresentMode{ vk::PresentModeKHR::eFifo };
        vk::Extent2D _desiredExtent{};
        vk::ImageUsageFlags _imageUsage{};

        uint32_t _graphicsFamilyIndex{ 0 };
        uint32_t _presentFamilyIndex{ 0 };
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::SwapChainBuilder& tpd::SwapChainBuilder::desiredSurfaceFormat(const vk::Format format, const vk::ColorSpaceKHR colorSpace) {
    _desiredFormat = format;
    _desiredColorSpace = colorSpace;
    return *this;
}

inline tpd::SwapChainBuilder& tpd::SwapChainBuilder::desiredPresentMode(const vk::PresentModeKHR presentMode) {
    _desiredPresentMode = presentMode;
    return *this;
}

inline tpd::SwapChainBuilder& tpd::SwapChainBuilder::desiredExtent(const uint32_t width, const uint32_t height) {
    _desiredExtent = vk::Extent2D{ width, height };
    return *this;
}

inline tpd::SwapChainBuilder& tpd::SwapChainBuilder::imageUsageFlags(const vk::ImageUsageFlags usage) {
    _imageUsage = usage;
    return *this;
}

inline tpd::SwapChainBuilder& tpd::SwapChainBuilder::queueFamilyIndices(const uint32_t graphicsQueue, const uint32_t presentQueue) {
    _graphicsFamilyIndex = graphicsQueue;
    _presentFamilyIndex = presentQueue;
    return *this;
}
