#pragma once

#include <vulkan/vulkan.hpp>

#include <optional>

namespace tpd {
    class PhysicalDeviceSelector {
    public:
        PhysicalDeviceSelector& deviceExtensions(const char* const* extensions, std::size_t count);
        PhysicalDeviceSelector& deviceExtensions(const std::vector<const char*>& extensions);
        PhysicalDeviceSelector& deviceExtensions(std::vector<const char*>&& extensions) noexcept;

        PhysicalDeviceSelector& requestGraphicsQueueFamily();
        PhysicalDeviceSelector& requestPresentQueueFamily(const vk::SurfaceKHR& surface);
        PhysicalDeviceSelector& requestComputeQueueFamily(bool distinct = false);

        PhysicalDeviceSelector& features(const vk::PhysicalDeviceFeatures& features);

        PhysicalDeviceSelector& featuresExtendedDynamicState(const vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT& features);
        PhysicalDeviceSelector& featuresExtendedDynamicState2(const vk::PhysicalDeviceExtendedDynamicState2FeaturesEXT& features);
        PhysicalDeviceSelector& featuresExtendedDynamicState3(const vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT& features);

        PhysicalDeviceSelector& featuresVulkan11(const vk::PhysicalDeviceVulkan11Features& features);
        PhysicalDeviceSelector& featuresVulkan12(const vk::PhysicalDeviceVulkan12Features& features);
        PhysicalDeviceSelector& featuresVulkan13(const vk::PhysicalDeviceVulkan13Features& features);

        PhysicalDeviceSelector& featuresDescriptorIndexing(const vk::PhysicalDeviceDescriptorIndexingFeatures& features);
        PhysicalDeviceSelector& featuresDynamicRendering(const vk::PhysicalDeviceDynamicRenderingFeatures& features);
        PhysicalDeviceSelector& featuresSynchronization2(const vk::PhysicalDeviceSynchronization2Features& features);
        PhysicalDeviceSelector& featuresTimelineSemaphore(const vk::PhysicalDeviceTimelineSemaphoreFeatures& features);
        PhysicalDeviceSelector& featuresConditionalRendering(const vk::PhysicalDeviceConditionalRenderingFeaturesEXT& features);
        PhysicalDeviceSelector& featuresVertexInputDynamicState(const vk::PhysicalDeviceVertexInputDynamicStateFeaturesEXT& features);

        [[nodiscard]] PhysicalDeviceSelector& select(const vk::Instance& instance);

        [[nodiscard]] vk::PhysicalDevice getPhysicalDevice() const;
        [[nodiscard]] uint32_t getGraphicsQueueFamily() const;
        [[nodiscard]] uint32_t getPresentQueueFamily() const;
        [[nodiscard]] uint32_t getComputeQueueFamily() const;

    private:
        std::vector<const char*> _deviceExtensions{};
        [[nodiscard]] bool checkExtensionSupport(const vk::PhysicalDevice& device) const;

        struct QueueFamilyIndices {
            std::optional<uint32_t> graphicsFamily{};
            std::optional<uint32_t> presentFamily{};
            std::optional<uint32_t> computeFamily{};
        };

        bool _requestGraphicsQueueFamily{ false };
        uint32_t _graphicsQueueFamily{ 0 };

        bool _requestPresentQueueFamily{ false };
        vk::SurfaceKHR _surface{};
        uint32_t _presentQueueFamily{ 0 };

        bool _requestComputeQueueFamily{ false };
        bool _distinctComputeQueueFamily{ false };
        uint32_t _computeQueueFamily{ 0 };

        vk::PhysicalDevice _physicalDevice{};
        bool _isSelected{ false };

        [[nodiscard]] QueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice& device) const;
        [[nodiscard]] bool queueFamiliesComplete(const QueueFamilyIndices& indices) const;

        [[nodiscard]] bool checkDeviceFeatures(const vk::PhysicalDevice& device) const;

        vk::PhysicalDeviceFeatures _features{};
        [[nodiscard]] bool checkFeatures(const vk::PhysicalDeviceFeatures& features) const;

        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT  _extendedDynamicState1Features{};
        vk::PhysicalDeviceExtendedDynamicState2FeaturesEXT _extendedDynamicState2Features{};
        vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT _extendedDynamicState3Features{};
        [[nodiscard]] bool checkExtendedDynamicState1Features(const vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT&  features) const;
        [[nodiscard]] bool checkExtendedDynamicState2Features(const vk::PhysicalDeviceExtendedDynamicState2FeaturesEXT& features) const;
        [[nodiscard]] bool checkExtendedDynamicState3Features(const vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT& features) const;

        vk::PhysicalDeviceVulkan11Features _vulkan11Features{};
        vk::PhysicalDeviceVulkan12Features _vulkan12Features{};
        vk::PhysicalDeviceVulkan13Features _vulkan13Features{};
        [[nodiscard]] bool checkVulkan11Features(const vk::PhysicalDeviceVulkan11Features& features) const;
        [[nodiscard]] bool checkVulkan12Features(const vk::PhysicalDeviceVulkan12Features& features) const;
        [[nodiscard]] bool checkVulkan13Features(const vk::PhysicalDeviceVulkan13Features& features) const;

        vk::PhysicalDeviceConditionalRenderingFeaturesEXT _conditionalRenderingFeatures{};
        [[nodiscard]] bool checkConditionalRenderingFeatures(const vk::PhysicalDeviceConditionalRenderingFeaturesEXT& features) const;

        vk::PhysicalDeviceVertexInputDynamicStateFeaturesEXT _vertexInputDynamicStateFeatures{};
        [[nodiscard]] bool checkVertexInputDynamicStateFeatures(const vk::PhysicalDeviceVertexInputDynamicStateFeaturesEXT& features) const;
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::PhysicalDeviceSelector& tpd::PhysicalDeviceSelector::deviceExtensions(const char * const* extensions, const std::size_t count) {
    _deviceExtensions = std::vector(extensions, extensions + count);
    return *this;
}

inline tpd::PhysicalDeviceSelector& tpd::PhysicalDeviceSelector::deviceExtensions(const std::vector<const char*>& extensions) {
    _deviceExtensions = extensions;
    return *this;
}

inline tpd::PhysicalDeviceSelector& tpd::PhysicalDeviceSelector::deviceExtensions(std::vector<const char*>&& extensions) noexcept {
    _deviceExtensions = std::move(extensions);
    return *this;
}

inline tpd::PhysicalDeviceSelector& tpd::PhysicalDeviceSelector::requestGraphicsQueueFamily() {
    _requestGraphicsQueueFamily = true;
    return *this;
}

inline tpd::PhysicalDeviceSelector& tpd::PhysicalDeviceSelector::requestPresentQueueFamily(const vk::SurfaceKHR& surface) {
    _surface = surface;
    _requestPresentQueueFamily = true;
    return *this;
}

inline tpd::PhysicalDeviceSelector& tpd::PhysicalDeviceSelector::requestComputeQueueFamily(const bool distinct) {
    _requestComputeQueueFamily = true;
    _distinctComputeQueueFamily = distinct;
    return *this;
}

inline tpd::PhysicalDeviceSelector& tpd::PhysicalDeviceSelector::features(const vk::PhysicalDeviceFeatures& features) {
    _features = features;
    return *this;
}

inline tpd::PhysicalDeviceSelector& tpd::PhysicalDeviceSelector::featuresExtendedDynamicState(
    const vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT& features) {
    _extendedDynamicState1Features = features;
    return *this;
}

inline tpd::PhysicalDeviceSelector& tpd::PhysicalDeviceSelector::featuresExtendedDynamicState2(
    const vk::PhysicalDeviceExtendedDynamicState2FeaturesEXT& features) {
    _extendedDynamicState2Features = features;
    return *this;
}

inline tpd::PhysicalDeviceSelector& tpd::PhysicalDeviceSelector::featuresExtendedDynamicState3(
    const vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT& features) {
    _extendedDynamicState3Features = features;
    return *this;
}

inline tpd::PhysicalDeviceSelector& tpd::PhysicalDeviceSelector::featuresVulkan11(
    const vk::PhysicalDeviceVulkan11Features& features) {
    _vulkan11Features = features;
    return *this;
}

inline tpd::PhysicalDeviceSelector& tpd::PhysicalDeviceSelector::featuresVulkan12(
    const vk::PhysicalDeviceVulkan12Features& features) {
    _vulkan12Features = features;
    return *this;
}

inline tpd::PhysicalDeviceSelector& tpd::PhysicalDeviceSelector::featuresVulkan13(
    const vk::PhysicalDeviceVulkan13Features& features) {
    _vulkan13Features = features;
    return *this;
}

inline tpd::PhysicalDeviceSelector& tpd::PhysicalDeviceSelector::featuresConditionalRendering(
    const vk::PhysicalDeviceConditionalRenderingFeaturesEXT& features) {
    _conditionalRenderingFeatures = features;
    return *this;
}

inline tpd::PhysicalDeviceSelector& tpd::PhysicalDeviceSelector::featuresVertexInputDynamicState(
    const vk::PhysicalDeviceVertexInputDynamicStateFeaturesEXT& features) {
    _vertexInputDynamicStateFeatures = features;
    return *this;
}

inline vk::PhysicalDevice tpd::PhysicalDeviceSelector::getPhysicalDevice() const {
    if (!_isSelected) {
        throw std::runtime_error(
            "PhysicalDevice has not been selected, did you forget to call PhysicalDeviceSelector::select()?");
    }
    return _physicalDevice;
}

inline uint32_t tpd::PhysicalDeviceSelector::getGraphicsQueueFamily() const {
    if (!_isSelected) {
        throw std::runtime_error(
            "PhysicalDevice has not been selected, did you forget to call PhysicalDeviceSelector::select()?");
    }
    if (!_requestGraphicsQueueFamily) {
        throw std::runtime_error(
            "Graphics queue family has not been requested, did you forget to call "
            "PhysicalDeviceSelector::requestGraphicsQueueFamily()?");
    }
    return _graphicsQueueFamily;
}

inline uint32_t tpd::PhysicalDeviceSelector::getPresentQueueFamily() const {
    if (!_isSelected) {
        throw std::runtime_error(
            "PhysicalDevice has not been selected, did you forget to call PhysicalDeviceSelector::select()?");
    }
    if (!_requestPresentQueueFamily) {
        throw std::runtime_error(
            "Present queue family has not been requested, did you forget to call "
            "PhysicalDeviceSelector::requestPresentQueueFamily()?");
    }
    return _presentQueueFamily;
}

inline uint32_t tpd::PhysicalDeviceSelector::getComputeQueueFamily() const {
    if (!_isSelected) {
        throw std::runtime_error(
            "PhysicalDevice has not been selected, did you forget to call PhysicalDeviceSelector::select()?");
    }
    if (!_requestComputeQueueFamily) {
        throw std::runtime_error(
            "Compute queue family has not been requested, did you forget to call "
            "PhysicalDeviceSelector::requestComputeQueueFamily()?");
    }
    return _computeQueueFamily;
}
