#include "TordemoApplication.h"

#include <plog/Log.h>

#include <torpedo/bootstrap/InstanceBuilder.h>
#include <torpedo/bootstrap/DeviceBuilder.h>

#include <limits>
#include <ranges>

void TordemoApplication::run() {
    initWindow();
    initVulkan();
    loop();
}

// =====================================================================================================================
// APPLICATION STAGES
// =====================================================================================================================

void TordemoApplication::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    _window = glfwCreateWindow(1280, 768, getWindowTitle().c_str(), nullptr, nullptr);

    glfwSetWindowUserPointer(_window, this);
    glfwSetFramebufferSizeCallback(_window, framebufferResizeCallback);
}

void TordemoApplication::initVulkan() {
    createInstance();
#ifndef NDEBUG
    createDebugMessenger();
#endif
    createSurface();
    pickPhysicalDevice();
    createDevice();
    initResourceAllocator();
    createSwapChain();
    createSwapChainImageViews();
    createFramebufferResources();
    createRenderPass();
    createFramebuffers();
    createCommandPools();
    createCommandBuffers();
    createSyncObjects();
    createPipelineResources();
}

void TordemoApplication::loop() {
    while (!glfwWindowShouldClose(_window)) {
        glfwPollEvents();
        drawFrame();
    }
    _device.waitIdle();
}

// =====================================================================================================================
// BYTE-SIZED OVERRIDES
// =====================================================================================================================

void TordemoApplication::createFramebufferResources() {
    createFramebufferColorResources();
    createFramebufferDepthResources();
}

void TordemoApplication::destroyFramebufferResources() const noexcept {
    _device.destroyImageView(_framebufferDepthImageView);
    _device.destroyImageView(_framebufferColorImageView);
    _resourceAllocator->destroyImage(_framebufferDepthImage, _framebufferDepthImageAllocation);
    _resourceAllocator->destroyImage(_framebufferColorImage, _framebufferColorImageAllocation);
}

void TordemoApplication::createCommandPools() {
    createDrawingCommandPool();
    createTransferCommandPool();
}

void TordemoApplication::destroyCommandPools() const noexcept {
    _device.destroyCommandPool(_drawingCommandPool);
    _device.destroyCommandPool(_transferCommandPool);
}

void TordemoApplication::createSyncObjects() {
    createDrawingSyncObjects();
}

void TordemoApplication::destroySyncObjects() const noexcept {
    using namespace std::ranges;
    for_each(_drawingInFlightFences, [this](const auto& it) { _device.destroyFence(it); });
    for_each(_renderFinishedSemaphores, [this](const auto& it) { _device.destroySemaphore(it); });
    for_each(_imageAvailableSemaphores, [this](const auto& it) { _device.destroySemaphore(it); });
}

void TordemoApplication::createPipelineResources() {
    createGraphicsPipelineShader();
}

void TordemoApplication::destroyPipelineResources() noexcept {
    _graphicsPipelineShader->dispose(_device);
}

void TordemoApplication::drawFrame() {
    uint32_t imageIndex;
    if (!beginFrame(&imageIndex)) return;

    onFrameReady();
    beginRenderPass(imageIndex);
    render(_drawingCommandBuffers[_currentFrame]);
    endFrame(imageIndex);

    _currentFrame = (_currentFrame + 1) % _maxFramesInFlight;
}

// =====================================================================================================================
// WINDOW
// =====================================================================================================================

std::string TordemoApplication::getWindowTitle() const {
    return "Hello, Tordemo!";
}

void TordemoApplication::framebufferResizeCallback(GLFWwindow* window, [[maybe_unused]] const int width, [[maybe_unused]] const int height) {
    const auto app = static_cast<TordemoApplication*>(glfwGetWindowUserPointer(window));
    app->_framebufferResized = true;
}

#ifndef NDEBUG
// =====================================================================================================================
// DEBUG MESSENGER
// =====================================================================================================================

VKAPI_ATTR vk::Bool32 VKAPI_CALL defaultDebugCallback(
    const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    [[maybe_unused]] const VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    [[maybe_unused]] void* const userData
) {
    switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            PLOGV << pCallbackData->pMessage; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            PLOGI << pCallbackData->pMessage; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            PLOGW << pCallbackData->pMessage; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            PLOGE << pCallbackData->pMessage; break;
        default: break;
    }

    return vk::False;
}

void TordemoApplication::createDebugMessenger() {
    const auto debugInfo = tpd::core::createDebugInfo(defaultDebugCallback);

    if (tpd::core::createDebugUtilsMessenger(_instance, &debugInfo, &_debugMessenger) == vk::Result::eErrorExtensionNotPresent) {
        throw std::runtime_error("Failed to set up a debug messenger!");
    }
}
#endif

// =====================================================================================================================
// VULKAN INSTANCE
// =====================================================================================================================

void TordemoApplication::createInstance() {
    auto instanceCreateFlags = vk::InstanceCreateFlags{};

#ifdef __APPLE__
    // Beginning with the 1.3.216 Vulkan SDK, the VK_KHR_PORTABILITY_subset extension is mandatory
    // for macOS with the latest MoltenVK SDK
    requiredExtensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
    instanceCreateFlags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif

    _instance = tpd::InstanceBuilder()
        .applicationInfo(getApplicationInfo())
#ifndef NDEBUG
        .debugInfo(tpd::core::createDebugInfo(defaultDebugCallback))
        .layers({ "VK_LAYER_KHRONOS_validation" })
#endif
        .extensions(getRequiredExtensions())
        .build(instanceCreateFlags);
}

std::vector<const char*> TordemoApplication::getRequiredExtensions() const {
    uint32_t glfwExtensionCount{ 0 };
    const auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    return { glfwExtensions, glfwExtensions + glfwExtensionCount };
}

vk::ApplicationInfo TordemoApplication::getApplicationInfo() const {
    return tpd::core::createApplicationInfo("tordemo", vk::makeApiVersion(0, 1, 3, 0));
}

// =====================================================================================================================
// VULKAN SURFACE
// =====================================================================================================================

void TordemoApplication::createSurface() {
    if (glfwCreateWindowSurface(_instance, _window, nullptr, reinterpret_cast<VkSurfaceKHR*>(&_surface)) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create a window surface!");
    }
}

// =====================================================================================================================
// PHYSICAL DEVICE AND QUEUE FAMILY INDICES
// =====================================================================================================================

void TordemoApplication::pickPhysicalDevice() {
    const auto selector = getPhysicalDeviceSelector().select(_instance);

    _graphicsQueueFamily = selector.getGraphicsQueueFamily();
    _presentQueueFamily = selector.getPresentQueueFamily();
    _computeQueueFamily = selector.getComputeQueueFamily();

    _physicalDevice = selector.getPhysicalDevice();
    PLOGI << "Found a suitable device: " << _physicalDevice.getProperties().deviceName.data();

#ifndef NDEBUG
    PLOGD << "Graphics queue family index: " << _graphicsQueueFamily;
    PLOGD << "Present queue family index:  " << _presentQueueFamily;
    PLOGD << "Compute queue family index:  " << _computeQueueFamily;
#endif
}

tpd::PhysicalDeviceSelector TordemoApplication::getPhysicalDeviceSelector() const {
    return tpd::PhysicalDeviceSelector()
        .deviceExtensions(getDeviceExtensions())
        .requestGraphicsQueueFamily()
        .requestPresentQueueFamily(_surface)
        .requestComputeQueueFamily();
}

std::vector<const char*> TordemoApplication::getDeviceExtensions() const {
    return {
        vk::KHRSwapchainExtensionName,
        vk::EXTMemoryBudgetExtensionName,    // help VMA estimate memory budget more accurately
        vk::EXTMemoryPriorityExtensionName,  // incorporate memory priority to the allocator
    };
}

// =====================================================================================================================
// DEVICE AND QUEUES
// =====================================================================================================================

void TordemoApplication::createDevice() {
    _device = tpd::DeviceBuilder()
        .deviceExtensions(getDeviceExtensions())
        .deviceFeatures(getDeviceFeatures())
        .queueFamilyIndices({ _graphicsQueueFamily, _presentQueueFamily, _computeQueueFamily })
        .build(_physicalDevice);

    _graphicsQueue = _device.getQueue(_graphicsQueueFamily, 0);
    _presentQueue = _device.getQueue(_presentQueueFamily, 0);
    _computeQueue = _device.getQueue(_computeQueueFamily, 0);
}

vk::PhysicalDeviceFeatures2 TordemoApplication::getDeviceFeatures() const {
    return {};
}

// =====================================================================================================================
// RESOURCE ALLOCATOR
// =====================================================================================================================

void TordemoApplication::initResourceAllocator() {
    _resourceAllocator = tpd::ResourceAllocator::Builder()
        .flags(VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT | VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT)
        .vulkanApiVersion(vk::makeApiVersion(0, 1, 3, 0))
        .build(_instance, _physicalDevice, _device);
}


// =====================================================================================================================
// SWAP CHAIN UTILITIES
// =====================================================================================================================

void TordemoApplication::recreateSwapChain() {
    // We won't recreate while the window is being minimized
    int width = 0, height = 0;
    glfwGetFramebufferSize(_window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(_window, &width, &height);
        glfwWaitEvents();
    }

    // Before cleaning, we shouldn’t touch resources that may still be in use
    _device.waitIdle();
    cleanupSwapChain();

    createSwapChain();
    createSwapChainImageViews();
    createFramebufferColorResources();
    createFramebufferDepthResources();
    createFramebuffers();

    // TODO: Make use of the oldSwapChain field
}

void TordemoApplication::cleanupSwapChain() const noexcept {
    destroyFramebufferResources();

    // Destroy the framebuffers and swap chain image views
    using namespace std::ranges;
    for_each(_framebuffers, [this](const auto& it) { _device.destroyFramebuffer(it); });
    for_each(_swapChainImageViews, [this](const auto& it) { _device.destroyImageView(it); });

    // Get rid of the old swap chain
    _device.destroySwapchainKHR(_swapChain);
}

bool TordemoApplication::acquireSwapChainImage(const vk::Semaphore semaphore, uint32_t* imageIndex) {
    using limits = std::numeric_limits<uint64_t>;
    const auto result = _device.acquireNextImageKHR(_swapChain, limits::max(), semaphore, nullptr);

    if (result.result == vk::Result::eErrorOutOfDateKHR) {
        recreateSwapChain();
        return false;
    }
    if (result.result != vk::Result::eSuccess && result.result != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("Failed to acquire a swap chain image");
    }

    *imageIndex = result.value;
    return true;
}

void TordemoApplication::presentSwapChainImage(uint32_t imageIndex, const vk::Semaphore semaphore) {
    const auto presentInfo = vk::PresentInfoKHR{ semaphore, _swapChain, imageIndex };

    if (const auto result = _presentQueue.presentKHR(&presentInfo);
        result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || _framebufferResized) {
        _framebufferResized = false;
        recreateSwapChain();
    } else if (result != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to present a swap chain image!");
    }
}

// =====================================================================================================================
// SWAP CHAIN INFRASTRUCTURE
// =====================================================================================================================

void TordemoApplication::createSwapChain() {
    const auto surfaceFormat = chooseSwapSurfaceFormat();
    const auto presentMode = chooseSwapPresentMode();
    const auto extent = chooseSwapExtent();

    const auto capabilities = _physicalDevice.getSurfaceCapabilitiesKHR(_surface);
    _minImageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && _minImageCount > capabilities.maxImageCount) {
        _minImageCount = capabilities.maxImageCount;
    }

    auto createInfo = vk::SwapchainCreateInfoKHR{};
    createInfo.surface = _surface;
    createInfo.minImageCount = _minImageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.presentMode = presentMode;
    createInfo.imageArrayLayers = 1;  // will always be 1 unless for a stereoscopic 3D application
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
    if (_graphicsQueueFamily != _presentQueueFamily) {
        // Concurrent mode requires us to specify in advance between which queue families ownership will be shared
        // using the queueFamilyIndexCount and pQueueFamilyIndices parameters
        const uint32_t queueFamilyIndices[]{ _graphicsQueueFamily, _presentQueueFamily };
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        // If the graphics queue family and presentation queue family are the same, which will be the case on most
        // hardware, then we should stick to exclusive mode
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.clipped = vk::True;
    createInfo.oldSwapchain = nullptr;

    _swapChain = _device.createSwapchainKHR(createInfo);
    _swapChainImages = _device.getSwapchainImagesKHR(_swapChain);
    _swapChainImageFormat = surfaceFormat.format;
    _swapChainImageExtent = extent;
}

vk::SurfaceFormatKHR TordemoApplication::chooseSwapSurfaceFormat() const {
    const auto availableFormats = _physicalDevice.getSurfaceFormatsKHR(_surface);

    constexpr auto format = vk::Format::eB8G8R8A8Srgb;
    constexpr auto colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;

    const auto suitable = [](const auto& it){ return it.format == format && it.colorSpace == colorSpace; };
    if (const auto found = std::ranges::find_if(availableFormats, suitable); found != availableFormats.end()) {
        return *found;
    }

    return availableFormats[0];
}

vk::PresentModeKHR TordemoApplication::chooseSwapPresentMode() const {
    const auto presentModes = _physicalDevice.getSurfacePresentModesKHR(_surface);

    constexpr auto preferredMode = vk::PresentModeKHR::eMailbox;
    if (const auto found = std::ranges::find(presentModes, preferredMode); found != presentModes.end()) {
        return preferredMode;
    }

    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D TordemoApplication::chooseSwapExtent() const {
    const auto capabilities = _physicalDevice.getSurfaceCapabilitiesKHR(_surface);
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    int width, height;
    glfwGetFramebufferSize(_window, &width, &height);

    auto actualExtent = vk::Extent2D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
    actualExtent.width = std::clamp(
        actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(
        actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

void TordemoApplication::createSwapChainImageViews() {
    using namespace std::ranges;

    const auto toImageView = [this](const auto& image) {
        constexpr auto aspectFlags = vk::ImageAspectFlagBits::eColor;
        constexpr auto mipLevels = 1;

        auto subresourceRange = vk::ImageSubresourceRange{};
        subresourceRange.aspectMask = aspectFlags;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = mipLevels;
        subresourceRange.layerCount = 1;

        auto viewInfo = vk::ImageViewCreateInfo{};
        viewInfo.image = image;
        viewInfo.format = _swapChainImageFormat;
        viewInfo.viewType = vk::ImageViewType::e2D;
        viewInfo.subresourceRange = subresourceRange;

        return _device.createImageView(viewInfo);
    };

    _swapChainImageViews = _swapChainImages | views::transform(toImageView) | to<std::vector>();
}

// =====================================================================================================================
// FRAMEBUFFER COLOR RESOURCES
// =====================================================================================================================

void TordemoApplication::createFramebufferColorResources() {
    using Usage = vk::ImageUsageFlagBits;
    constexpr auto mipLevels = 1;

    auto imageCreateInfo = vk::ImageCreateInfo{};
    imageCreateInfo.imageType = vk::ImageType::e2D;
    imageCreateInfo.format = _swapChainImageFormat;
    imageCreateInfo.extent.width = _swapChainImageExtent.width;
    imageCreateInfo.extent.height = _swapChainImageExtent.height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = mipLevels;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = getFramebufferColorImageSampleCount();
    imageCreateInfo.tiling = vk::ImageTiling::eOptimal;
    imageCreateInfo.usage = Usage::eTransientAttachment | Usage::eColorAttachment;
    imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    imageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;

    _framebufferColorImage = _resourceAllocator->allocateDedicatedImage(imageCreateInfo, &_framebufferColorImageAllocation);

    constexpr auto aspectFlags = vk::ImageAspectFlagBits::eColor;
    auto subresourceRange = vk::ImageSubresourceRange{};
    subresourceRange.aspectMask = aspectFlags;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = mipLevels;
    subresourceRange.layerCount = 1;

    auto imageViewCreateInfo = vk::ImageViewCreateInfo{};
    imageViewCreateInfo.image = _framebufferColorImage;
    imageViewCreateInfo.format = _swapChainImageFormat;
    imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
    imageViewCreateInfo.subresourceRange = subresourceRange;

    _framebufferColorImageView = _device.createImageView(imageViewCreateInfo);
}

vk::SampleCountFlagBits TordemoApplication::getFramebufferColorImageSampleCount() const {
    return getOrFallbackSampleCount(vk::SampleCountFlagBits::e4);
}

uint32_t TordemoApplication::getFramebufferColorImageMipLevels() const {
    return 1;
}

// =====================================================================================================================
// FRAMEBUFFER DEPTH RESOURCES
// =====================================================================================================================

void TordemoApplication::createFramebufferDepthResources() {
    const auto formatCandidates = std::vector{
        vk::Format::eD32Sfloat,        // 32-bit float for depth
        vk::Format::eD32SfloatS8Uint,  // 32-bit signed float for depth and 8-bit stencil component
        vk::Format::eD24UnormS8Uint,   // 24-bit float for depth and 8-bit stencil component
    };

    // Choose the desired depth format
    using Tiling = vk::ImageTiling;
    using Feature = vk::FormatFeatureFlagBits;
    _framebufferDepthImageFormat = findFramebufferDepthImageFormat(formatCandidates, Tiling::eOptimal, Feature::eDepthStencilAttachment);

    constexpr auto mipLevels = 1;
    constexpr auto aspectFlags = vk::ImageAspectFlagBits::eDepth;

    auto imageCreateInfo = vk::ImageCreateInfo{};
    imageCreateInfo.imageType = vk::ImageType::e2D;
    imageCreateInfo.format = _framebufferDepthImageFormat;
    imageCreateInfo.extent.width = _swapChainImageExtent.width;
    imageCreateInfo.extent.height = _swapChainImageExtent.height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = mipLevels;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = getFramebufferDepthImageSampleCount();
    imageCreateInfo.tiling = vk::ImageTiling::eOptimal;
    imageCreateInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
    imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    imageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;

    _framebufferDepthImage = _resourceAllocator->allocateDedicatedImage(imageCreateInfo, &_framebufferDepthImageAllocation);

    auto subresourceRange = vk::ImageSubresourceRange{};
    subresourceRange.aspectMask = aspectFlags;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = mipLevels;
    subresourceRange.layerCount = 1;

    auto imageViewCreateInfo = vk::ImageViewCreateInfo{};
    imageViewCreateInfo.image = _framebufferDepthImage;
    imageViewCreateInfo.format = _framebufferDepthImageFormat;
    imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
    imageViewCreateInfo.subresourceRange = subresourceRange;

    _framebufferDepthImageView = _device.createImageView(imageViewCreateInfo);
}

vk::Format TordemoApplication::findFramebufferDepthImageFormat(
    const std::vector<vk::Format> &candidates,
    const vk::ImageTiling tiling,
    const vk::FormatFeatureFlags features) const
{
    // The support of a format depends on the tiling mode and usage, so we must also include these in the check
    for (const auto format : candidates) {
        const auto props = _physicalDevice.getFormatProperties(format);
        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("Failed to find a supported framebuffer depth image format");
}

vk::SampleCountFlagBits TordemoApplication::getFramebufferDepthImageSampleCount() const {
    return getOrFallbackSampleCount(vk::SampleCountFlagBits::e4);
}

// =====================================================================================================================
// RENDER PASS
// =====================================================================================================================

void TordemoApplication::createRenderPass() {
    const auto colorAttachment = vk::AttachmentDescription{
        {}, _swapChainImageFormat,
        getFramebufferColorImageSampleCount(),
        vk::AttachmentLoadOp::eClear,   // clear the framebuffer to black before drawing a new frame
        vk::AttachmentStoreOp::eStore,  // rendered contents will be stored in memory to be resolved later
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal
    };
    const auto depthAttachment = vk::AttachmentDescription{
        {}, _framebufferDepthImageFormat,
        getFramebufferDepthImageSampleCount(),
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eDontCare,  // we're not using depth values after drawing has finished
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal
    };
    const auto colorAttachmentResolve = vk::AttachmentDescription{
        {}, _swapChainImageFormat,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eStore,    // resolved contents will be stored in memory to be presented later
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::ePresentSrcKHR   // we want the image to be ready for presentation after rendering
    };

    // We should keep the order we specify these attachments in mind because later on we will have to specify
    // clear values for them in the same order. Also, the attachment reference objects reference attachments using the
    // indices of this array
    const auto attachments = std::array{ colorAttachment, depthAttachment, colorAttachmentResolve };

    static constexpr auto colorAttachmentRef = vk::AttachmentReference{ 0, vk::ImageLayout::eColorAttachmentOptimal };
    static constexpr auto depthAttachmentRef = vk::AttachmentReference{ 1, vk::ImageLayout::eDepthStencilAttachmentOptimal };
    static constexpr auto colorAttachmentResolveRef = vk::AttachmentReference{ 2, vk::ImageLayout::eColorAttachmentOptimal };

    // Define a subpass for our rendering
    auto subpass = vk::SubpassDescription{};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    // The index of the attachment in this array is directly referenced from the fragment shader with the
    // "layout(location = <index>) out" directive
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    // Unlike color attachments, a subpass can only use a single depth (+stencil) attachment
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    // This is enough to let the render pass define a multisample resolve operation
    subpass.pResolveAttachments = &colorAttachmentResolveRef;

    // Subpasses in a render pass automatically take care of image layout transitions. These transitions are controlled
    // by subpass dependencies, which specify memory and execution dependencies between subpasses. We have only a single
    // subpass right now, but the operations right before and right after this subpass also count as implicit "subpasses"
    // There are two built-in dependencies that take care of the transition at the start of the render pass and at the
    // end of the render pass, but the former does not occur at the right time. It assumes that the transition occurs
    // at the start of the pipeline, but we haven’t acquired the image yet at that point because the Renderer uses
    // eColorAttachmentOutput for the drawing wait stage. There are two ways to deal with this problem:
    // - In the Renderer, change the waitStages for the imageAvailableSemaphore to eTopOfPipe to ensure that the render
    // passes don’t begin until the image is available
    // - Make the render pass wait for the eColorAttachmentOutput stage, using subpass dependency
    using Stage = vk::PipelineStageFlagBits;
    using Access = vk::AccessFlagBits;
    constexpr auto srcStages = Stage::eColorAttachmentOutput | Stage::eLateFragmentTests;
    constexpr auto dstStages = Stage::eColorAttachmentOutput | Stage::eEarlyFragmentTests;
    constexpr auto srcAccess = Access::eColorAttachmentWrite | Access::eDepthStencilAttachmentWrite;
    constexpr auto dstAccess = Access::eColorAttachmentWrite | Access::eDepthStencilAttachmentWrite;
    constexpr auto dependency = vk::SubpassDependency{
        vk::SubpassExternal,  // src subpass
        0,                    // dst subpass
        srcStages, dstStages,
        srcAccess, dstAccess,
    };

    const auto renderPassInfo = vk::RenderPassCreateInfo{ {}, attachments, subpass, dependency };
    _renderPass = _device.createRenderPass(renderPassInfo);
}

// =====================================================================================================================
// FRAMEBUFFERS
// =====================================================================================================================

void TordemoApplication::createFramebuffers() {
    const auto toFramebuffer = [this](const auto& swapChainImageView) {
        // Specify the image views that should be bound to the respective attachment descriptions in the render pass
        const auto attachments = std::array{ _framebufferColorImageView, _framebufferDepthImageView, swapChainImageView };

        auto framebufferInfo = vk::FramebufferCreateInfo{};
        framebufferInfo.renderPass = _renderPass;
        framebufferInfo.attachmentCount = attachments.size();
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = _swapChainImageExtent.width;
        framebufferInfo.height = _swapChainImageExtent.height;
        framebufferInfo.layers = 1;  // our swap chain images are single images, so the number of layers is 1

        return _device.createFramebuffer(framebufferInfo);
    };

    // The swap chain attachment differs for every swap chain image, but the same color and depth image can be used by
    // all of them because only a single subpass is running at the same time due to our semaphores.
    _framebuffers = _swapChainImageViews | std::ranges::views::transform(toFramebuffer) | std::ranges::to<std::vector>();
}

vk::SampleCountFlagBits TordemoApplication::getOrFallbackSampleCount(const vk::SampleCountFlagBits sampleCount) const {
    const auto physicalDeviceProperties = _physicalDevice.getProperties();
    const auto counts = physicalDeviceProperties.limits.framebufferColorSampleCounts &
        physicalDeviceProperties.limits.framebufferDepthSampleCounts;

    // Get max usable sample count
    using enum vk::SampleCountFlagBits;
    auto maxUsableSampleCount = e1;
    if      (counts & e64) { maxUsableSampleCount = e64; }
    else if (counts & e32) { maxUsableSampleCount = e32; }
    else if (counts & e16) { maxUsableSampleCount = e16; }
    else if (counts & e8)  { maxUsableSampleCount = e8; }
    else if (counts & e4)  { maxUsableSampleCount = e4; }
    else if (counts & e2)  { maxUsableSampleCount = e2; }

    // If the sample count is supported, return it
    if (sampleCount <= maxUsableSampleCount) {
        return sampleCount;
    }
    // Otherwise fallback to the max usable sample count
    switch (maxUsableSampleCount) {
        case e2:  PLOGW << "Falling back MSAA configuration: your device only supports up to 2x MSAA";  return e2;
        case e4:  PLOGW << "Falling back MSAA configuration: your device only supports up to 4x MSAA";  return e4;
        case e8:  PLOGW << "Falling back MSAA configuration: your device only supports up to 8x MSAA";  return e8;
        case e16: PLOGW << "Falling back MSAA configuration: your device only supports up to 16x MSAA"; return e16;
        case e32: PLOGW << "Falling back MSAA configuration: your device only supports up to 32x MSAA"; return e32;
        case e64: PLOGW << "Falling back MSAA configuration: your device only supports up to 64x MSAA"; return e64;
        default:  PLOGW << "Falling back MSAA configuration: your device does not support MSAA"; return  e1;
    }
}

// =====================================================================================================================
// COMMAND POOLS AND COMMAND BUFFERS
// =====================================================================================================================

void TordemoApplication::createDrawingCommandPool() {
    auto poolInfo = vk::CommandPoolCreateInfo{};
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    poolInfo.queueFamilyIndex = _graphicsQueueFamily;

    _drawingCommandPool = _device.createCommandPool(poolInfo);
}

void TordemoApplication::createTransferCommandPool() {
    const auto transferPoolInfo = vk::CommandPoolCreateInfo{ vk::CommandPoolCreateFlagBits::eTransient, _graphicsQueueFamily };
    _transferCommandPool = _device.createCommandPool(transferPoolInfo);
}

void TordemoApplication::createCommandBuffers() {
    // Allocate a drawing command buffer for each in-flight frame
    auto allocInfo = vk::CommandBufferAllocateInfo{};
    allocInfo.commandPool = _drawingCommandPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = _maxFramesInFlight;
    _drawingCommandBuffers = _device.allocateCommandBuffers(allocInfo);
}

// =====================================================================================================================
// SYNC OBJECTS
// =====================================================================================================================

void TordemoApplication::createDrawingSyncObjects() {
    _imageAvailableSemaphores.resize(_maxFramesInFlight);
    _renderFinishedSemaphores.resize(_maxFramesInFlight);
    _drawingInFlightFences.resize(_maxFramesInFlight);

    for (size_t i = 0; i < _maxFramesInFlight; i++) {
        _imageAvailableSemaphores[i] = _device.createSemaphore({});
        _renderFinishedSemaphores[i] = _device.createSemaphore({});
        _drawingInFlightFences[i] = _device.createFence({ vk::FenceCreateFlagBits::eSignaled });
    }
}

// =====================================================================================================================
// GRAPHICS PIPELINE
// =====================================================================================================================

void TordemoApplication::createGraphicsPipelineShader() {
    // Specify which properties will be able to change at runtime
    const auto dynamicStates = getGraphicsPipelineDynamicStates();
    auto dynamicStateInfo = vk::PipelineDynamicStateCreateInfo{};
    dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicStateInfo.pDynamicStates = dynamicStates.data();

    // Default viewport state
    const auto viewportState = getGraphicsPipelineViewportState();
    // Describe what kind of geometry will be drawn from the vertices and if primitive restart should be enabled
    const auto inputAssembly = getGraphicsPipelineInputAssemblyState();
    // Input vertex binding and attribute descriptions
    const auto vertexInputState = getGraphicsPipelineVertexInputState();
    // Rasterizer options
    const auto rasterizer = getGraphicsPipelineRasterizationState();
    // MSAA
    const auto multisampling = getGraphicsPipelineMultisampleState();
    // Depth stencil options
    const auto depthStencil = getGraphicsPipelineDepthStencilState();
    // How the newly computed fragment colors are combined with the color that is already in the framebuffer
    const auto colorBlendAttachment = getGraphicsPipelineColorBlendAttachmentState();
    // Configure global color blending
    const auto colorBlending = vk::PipelineColorBlendStateCreateInfo{ {}, vk::False, {}, 1, &colorBlendAttachment };

    // Construct the pipeline
    auto pipelineInfo = vk::GraphicsPipelineCreateInfo{};
    // Fixed and dynamic states
    pipelineInfo.pVertexInputState = &vertexInputState;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicStateInfo;
    // Reference to the render pass and the index of the subpass where this graphics pipeline will be used.
    // It is also possible to use other render passes with this pipeline instead of this specific instance,
    // but they have to be compatible with this very specific renderPass
    pipelineInfo.renderPass = _renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = nullptr;
    pipelineInfo.basePipelineIndex = -1;

    _graphicsPipelineShader = buildPipelineShader(&pipelineInfo);
}

std::vector<vk::DynamicState> TordemoApplication::getGraphicsPipelineDynamicStates() const {
    return { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
}

vk::PipelineViewportStateCreateInfo TordemoApplication::getGraphicsPipelineViewportState() const {
    return vk::PipelineViewportStateCreateInfo{ {}, 1, {}, 1, {} };
}

vk::PipelineInputAssemblyStateCreateInfo TordemoApplication::getGraphicsPipelineInputAssemblyState() const {
    return vk::PipelineInputAssemblyStateCreateInfo{ {}, vk::PrimitiveTopology::eTriangleList, vk::False };
}

vk::PipelineVertexInputStateCreateInfo TordemoApplication::getGraphicsPipelineVertexInputState() const {
    return vk::PipelineVertexInputStateCreateInfo{ {}, 0, nullptr, 0, nullptr };
}

vk::PipelineRasterizationStateCreateInfo TordemoApplication::getGraphicsPipelineRasterizationState() const {
    auto rasterizer = vk::PipelineRasterizationStateCreateInfo{};
    rasterizer.depthBiasEnable = vk::False;
    rasterizer.rasterizerDiscardEnable = vk::False;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
    rasterizer.depthBiasEnable = vk::False;
    rasterizer.lineWidth = 1.0f;
    return rasterizer;
}

vk::PipelineMultisampleStateCreateInfo TordemoApplication::getGraphicsPipelineMultisampleState() const {
    auto multisampling = vk::PipelineMultisampleStateCreateInfo{};
    multisampling.rasterizationSamples = getFramebufferColorImageSampleCount();
    multisampling.sampleShadingEnable = vk::False;
    multisampling.alphaToCoverageEnable = vk::False;
    multisampling.alphaToOneEnable = vk::False;
    return multisampling;
}

vk::PipelineDepthStencilStateCreateInfo TordemoApplication::getGraphicsPipelineDepthStencilState() const {
    auto depthStencil = vk::PipelineDepthStencilStateCreateInfo{};
    depthStencil.depthTestEnable = vk::True;
    depthStencil.depthWriteEnable = vk::True;
    depthStencil.depthCompareOp = vk::CompareOp::eLess;
    // Optional depth bound test
    depthStencil.depthBoundsTestEnable = vk::False;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
    // Optional stencil test
    depthStencil.stencilTestEnable = vk::False;
    depthStencil.front = vk::StencilOp::eZero;
    depthStencil.back = vk::StencilOp::eZero;
    return depthStencil;
}

vk::PipelineColorBlendAttachmentState TordemoApplication::getGraphicsPipelineColorBlendAttachmentState() const {
    auto colorBlendAttachment = vk::PipelineColorBlendAttachmentState{};
    colorBlendAttachment.blendEnable = vk::False;
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR |
                                          vk::ColorComponentFlagBits::eG |
                                          vk::ColorComponentFlagBits::eB |
                                          vk::ColorComponentFlagBits::eA;
    return colorBlendAttachment;
}

std::unique_ptr<tpd::PipelineShader> TordemoApplication::buildPipelineShader(vk::GraphicsPipelineCreateInfo* pipelineInfo) const {
    return tpd::PipelineShader::Builder()
        .shader("assets/shaders/blank.vert.spv", vk::ShaderStageFlagBits::eVertex)
        .shader("assets/shaders/blank.frag.spv", vk::ShaderStageFlagBits::eFragment)
        .build(pipelineInfo, _physicalDevice, _device);
}

// =====================================================================================================================
// DRAWINGS
// =====================================================================================================================

bool TordemoApplication::beginFrame(uint32_t* imageIndex) {
    using limits = std::numeric_limits<uint64_t>;
    [[maybe_unused]] const auto result = _device.waitForFences(_drawingInFlightFences[_currentFrame], vk::True, limits::max());

    if (!acquireSwapChainImage(_imageAvailableSemaphores[_currentFrame], imageIndex)) {
        return false;
    }

    _device.resetFences(_drawingInFlightFences[_currentFrame]);

    return true;
}

void TordemoApplication::onFrameReady() {
    _drawingCommandBuffers[_currentFrame].reset();
    _drawingCommandBuffers[_currentFrame].begin(vk::CommandBufferBeginInfo{});
}

void TordemoApplication::endFrame(const uint32_t imageIndex) {
    _drawingCommandBuffers[_currentFrame].endRenderPass();
    _drawingCommandBuffers[_currentFrame].end();

    const auto waitSemaphores = std::array{ _imageAvailableSemaphores[_currentFrame] };
    constexpr vk::PipelineStageFlags waitStages[]{ vk::PipelineStageFlagBits::eColorAttachmentOutput };

    auto drawingSubmitInfo = vk::SubmitInfo{};
    drawingSubmitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
    drawingSubmitInfo.pWaitSemaphores = waitSemaphores.data();
    drawingSubmitInfo.pWaitDstStageMask = waitStages;
    drawingSubmitInfo.commandBufferCount = 1;
    drawingSubmitInfo.pCommandBuffers = &_drawingCommandBuffers[_currentFrame];
    drawingSubmitInfo.signalSemaphoreCount = 1;
    drawingSubmitInfo.pSignalSemaphores = &_renderFinishedSemaphores[_currentFrame];

    _graphicsQueue.submit(drawingSubmitInfo, _drawingInFlightFences[_currentFrame]);

    presentSwapChainImage(imageIndex, _renderFinishedSemaphores[_currentFrame]);
}

void TordemoApplication::beginRenderPass(const uint32_t imageIndex) {
    auto renderPassBeginInfo = vk::RenderPassBeginInfo{};
    renderPassBeginInfo.renderPass = _renderPass;
    renderPassBeginInfo.framebuffer = _framebuffers[imageIndex];
    renderPassBeginInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
    renderPassBeginInfo.renderArea.extent = _swapChainImageExtent;
    const auto clearValues = getClearValues();
    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassBeginInfo.pClearValues = clearValues.data();

    _drawingCommandBuffers[_currentFrame].beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
}

std::vector<vk::ClearValue> TordemoApplication::getClearValues() const {
    return {
        vk::ClearColorValue{ 0.0f, 0.0f, 0.0f, 1.0f },                    // for the multi-sampled color attachment
        vk::ClearDepthStencilValue{ /* depth */ 1.0f, /* stencil */ 0 },  // for the depth/stencil attachment
    };
}

void TordemoApplication::render(const vk::CommandBuffer buffer) {
    buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _graphicsPipelineShader->getPipeline());

    // Set viewport and scissor states
    auto viewport = vk::Viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(_swapChainImageExtent.width);
    viewport.height = static_cast<float>(_swapChainImageExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    buffer.setViewport(0, viewport);
    buffer.setScissor(0, vk::Rect2D{ { 0, 0 }, _swapChainImageExtent });

    // Override to issue draw calls
}

// =====================================================================================================================
// TRANSFER OPERATIONS
// =====================================================================================================================

vk::CommandBuffer TordemoApplication::beginSingleTimeTransferCommands() const {
    const auto allocInfo = vk::CommandBufferAllocateInfo{ _transferCommandPool, vk::CommandBufferLevel::ePrimary, 1 };
    const auto commandBuffer = _device.allocateCommandBuffers(allocInfo)[0];

    // Use the command buffer once and wait with returning from the function until the copy operation has finished
    constexpr auto beginInfo = vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
    commandBuffer.begin(beginInfo);

    return commandBuffer;
}

void TordemoApplication::endSingleTimeTransferCommands(const vk::CommandBuffer commandBuffer) const {
    commandBuffer.end();
    _graphicsQueue.submit(vk::SubmitInfo{ {}, {}, {}, 1, &commandBuffer });
    _graphicsQueue.waitIdle();
    _device.freeCommandBuffers(_transferCommandPool, 1, &commandBuffer);
}

TordemoApplication::~TordemoApplication() {
    TordemoApplication::destroyPipelineResources();
    TordemoApplication::destroySyncObjects();
    TordemoApplication::destroyCommandPools();
    _device.destroyRenderPass(_renderPass);
    cleanupSwapChain();
    _resourceAllocator.reset();
    _device.destroy();
    _instance.destroySurfaceKHR(_surface);
#ifndef NDEBUG
    tpd::core::destroyDebugUtilsMessenger(_instance, _debugMessenger);
#endif
    _instance.destroy();

    glfwDestroyWindow(_window);
    glfwTerminate();
}
