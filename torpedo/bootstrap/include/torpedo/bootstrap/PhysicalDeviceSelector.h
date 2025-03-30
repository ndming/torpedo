#pragma once

#include <vulkan/vulkan.hpp>

#include <optional>

namespace tpd {
    struct PhysicalDeviceSelection {
        vk::PhysicalDevice physicalDevice{};
        uint32_t graphicsQueueFamilyIndex{};
        uint32_t transferQueueFamilyIndex{};
        uint32_t presentQueueFamilyIndex{};
        uint32_t computeQueueFamilyIndex{};
    };

    class PhysicalDeviceSelector {
    public:
        PhysicalDeviceSelector& requestGraphicsQueueFamily() noexcept;
        PhysicalDeviceSelector& requestTransferQueueFamily() noexcept;
        PhysicalDeviceSelector& requestPresentQueueFamily(vk::SurfaceKHR surface) noexcept;
        PhysicalDeviceSelector& requestAsyncComputeFamily() noexcept;

        PhysicalDeviceSelector& features(const vk::PhysicalDeviceFeatures& features);

        PhysicalDeviceSelector& featuresExtendedDynamicState(const vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT& features);
        PhysicalDeviceSelector& featuresExtendedDynamicState2(const vk::PhysicalDeviceExtendedDynamicState2FeaturesEXT& features);
        PhysicalDeviceSelector& featuresExtendedDynamicState3(const vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT& features);

        PhysicalDeviceSelector& featuresVulkan11(const vk::PhysicalDeviceVulkan11Features& features);
        PhysicalDeviceSelector& featuresVulkan12(const vk::PhysicalDeviceVulkan12Features& features);
        PhysicalDeviceSelector& featuresVulkan13(const vk::PhysicalDeviceVulkan13Features& features);
        PhysicalDeviceSelector& featuresVulkan14(const vk::PhysicalDeviceVulkan14Features& features);

        PhysicalDeviceSelector& featuresDescriptorIndexing(const vk::PhysicalDeviceDescriptorIndexingFeatures& features);
        PhysicalDeviceSelector& featuresDynamicRendering(const vk::PhysicalDeviceDynamicRenderingFeatures& features);
        PhysicalDeviceSelector& featuresSynchronization2(const vk::PhysicalDeviceSynchronization2Features& features);
        PhysicalDeviceSelector& featuresTimelineSemaphore(const vk::PhysicalDeviceTimelineSemaphoreFeatures& features);
        PhysicalDeviceSelector& featuresConditionalRendering(const vk::PhysicalDeviceConditionalRenderingFeaturesEXT& features);
        PhysicalDeviceSelector& featuresVertexInputDynamicState(const vk::PhysicalDeviceVertexInputDynamicStateFeaturesEXT& features);

        [[nodiscard]] PhysicalDeviceSelection select(vk::Instance instance, const std::vector<const char*>& extensions = {}) const;

    private:
        static bool checkExtensionSupport(vk::PhysicalDevice physicalDevice, const std::vector<const char*>& extensions);

        struct QueueFamilyIndices {
            std::optional<uint32_t> graphicsFamily{};
            std::optional<uint32_t> transferFamily{};
            std::optional<uint32_t> presentFamily{};
            std::optional<uint32_t> computeFamily{};
        };

        bool _requestGraphicsQueueFamily{ false };
        bool _requestTransferQueueFamily{ false };

        bool _requestPresentQueueFamily{ false };
        vk::SurfaceKHR _surface{};

        bool _asyncCompute{ false };

        [[nodiscard]] QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device) const;
        [[nodiscard]] bool queueFamiliesComplete(const QueueFamilyIndices& indices) const;

        [[nodiscard]] bool checkPhysicalDeviceFeatures(vk::PhysicalDevice physicalDevice) const;

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
        vk::PhysicalDeviceVulkan14Features _vulkan14Features{};
        [[nodiscard]] bool checkVulkan11Features(const vk::PhysicalDeviceVulkan11Features& features) const;
        [[nodiscard]] bool checkVulkan12Features(const vk::PhysicalDeviceVulkan12Features& features) const;
        [[nodiscard]] bool checkVulkan13Features(const vk::PhysicalDeviceVulkan13Features& features) const;
        [[nodiscard]] bool checkVulkan14Features(const vk::PhysicalDeviceVulkan14Features& features) const;

        vk::PhysicalDeviceDescriptorIndexingFeatures _descriptorIndexingFeatures{};
        [[nodiscard]] bool checkDescriptorIndexingFeatures(const vk::PhysicalDeviceDescriptorIndexingFeaturesEXT& features) const;

        vk::PhysicalDeviceDynamicRenderingFeatures _dynamicRenderingFeatures{};
        [[nodiscard]] bool checkDynamicRenderingFeatures(const vk::PhysicalDeviceDynamicRenderingFeatures& features) const;

        vk::PhysicalDeviceSynchronization2Features _synchronization2Features{};
        [[nodiscard]] bool checkSynchronization2Features(const vk::PhysicalDeviceSynchronization2Features& features) const;

        vk::PhysicalDeviceTimelineSemaphoreFeatures _timelineSemaphoreFeatures{};
        [[nodiscard]] bool checkTimelineSemaphoreFeatures(const vk::PhysicalDeviceTimelineSemaphoreFeatures& features) const;

        vk::PhysicalDeviceConditionalRenderingFeaturesEXT _conditionalRenderingFeatures{};
        [[nodiscard]] bool checkConditionalRenderingFeatures(const vk::PhysicalDeviceConditionalRenderingFeaturesEXT& features) const;

        vk::PhysicalDeviceVertexInputDynamicStateFeaturesEXT _vertexInputDynamicStateFeatures{};
        [[nodiscard]] bool checkVertexInputDynamicStateFeatures(const vk::PhysicalDeviceVertexInputDynamicStateFeaturesEXT& features) const;
    };
}  // namespace tpd

inline tpd::PhysicalDeviceSelector& tpd::PhysicalDeviceSelector::requestGraphicsQueueFamily() noexcept {
    _requestGraphicsQueueFamily = true;
    return *this;
}

inline tpd::PhysicalDeviceSelector& tpd::PhysicalDeviceSelector::requestTransferQueueFamily() noexcept {
    _requestTransferQueueFamily = true;
    return *this;
}

inline tpd::PhysicalDeviceSelector& tpd::PhysicalDeviceSelector::requestPresentQueueFamily(const vk::SurfaceKHR surface) noexcept {
    _surface = surface;
    _requestPresentQueueFamily = true;
    return *this;
}

inline tpd::PhysicalDeviceSelector& tpd::PhysicalDeviceSelector::requestAsyncComputeFamily() noexcept {
    _asyncCompute = true;
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

inline tpd::PhysicalDeviceSelector& tpd::PhysicalDeviceSelector::featuresVulkan14(
    const vk::PhysicalDeviceVulkan14Features& features) {
    _vulkan14Features = features;
    return *this;
}

inline tpd::PhysicalDeviceSelector& tpd::PhysicalDeviceSelector::featuresDescriptorIndexing(
    const vk::PhysicalDeviceDescriptorIndexingFeatures& features) {
    _descriptorIndexingFeatures = features;
    return *this;
}

inline tpd::PhysicalDeviceSelector& tpd::PhysicalDeviceSelector::featuresDynamicRendering(
    const vk::PhysicalDeviceDynamicRenderingFeatures& features) {
    _dynamicRenderingFeatures = features;
    return *this;
}

inline tpd::PhysicalDeviceSelector& tpd::PhysicalDeviceSelector::featuresSynchronization2(
    const vk::PhysicalDeviceSynchronization2Features& features) {
    _synchronization2Features = features;
    return *this;
}

inline tpd::PhysicalDeviceSelector& tpd::PhysicalDeviceSelector::featuresTimelineSemaphore(
    const vk::PhysicalDeviceTimelineSemaphoreFeatures& features) {
    _timelineSemaphoreFeatures = features;
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
