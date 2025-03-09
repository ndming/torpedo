#include "torpedo/rendering/StandardRenderer.h"

#include <torpedo/bootstrap/SwapChainBuilder.h>
#include <torpedo/bootstrap/PhysicalDeviceSelector.h>
#include <torpedo/bootstrap/DebugUtils.h>

#include <plog/Log.h>

#include <chrono>
#include <iostream>
#include <ranges>

tpd::StandardRenderer::Context::Context(const vk::Extent2D initialFramebufferSize) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    const auto w = static_cast<int>(initialFramebufferSize.width);
    const auto h = static_cast<int>(initialFramebufferSize.height);
    _window = glfwCreateWindow(w, h, "torpedo", nullptr, nullptr);
}

void tpd::StandardRenderer::Context::loop(const std::function<void()>& onRender) const {
    while (!glfwWindowShouldClose(_window)) [[likely]] {
        glfwPollEvents();
        onRender();
    }
}

void tpd::StandardRenderer::Context::loop(const std::function<void(float)>& onRender) const {
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (!glfwWindowShouldClose(_window)) [[likely]] {
        glfwPollEvents();

        const auto currentTime = std::chrono::high_resolution_clock::now();
        const float deltaTimeMillis = std::chrono::duration<float, std::milli>(currentTime - lastTime).count();
        onRender(deltaTimeMillis);

        lastTime = currentTime;
    }
}

tpd::StandardRenderer::Context::~Context() {
    glfwDestroyWindow(_window);
    glfwTerminate();
}

void tpd::StandardRenderer::init(const uint32_t framebufferWidth, const uint32_t framebufferHeight) {
    if (!_initialized) [[likely]] {
        PLOGI << "Initializing renderer: PresentRenderer";

        _context = std::make_unique<Context>(vk::Extent2D{ framebufferWidth, framebufferHeight });
        glfwSetWindowUserPointer(_context->_window, this);
        glfwSetFramebufferSizeCallback(_context->_window, framebufferResizeCallback);

        Renderer::init();
        createSurface();
    } else {
        PLOGI << "Skipping already initialized renderer: PresentRenderer";
    }
}

std::vector<const char*> tpd::StandardRenderer::getInstanceExtensions() const {
    // Override to add extensions from GLFW
    uint32_t glfwExtensionCount{ 0 };
    const auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    // We must also append extensions from parents, if any
    auto parentExtensions = Renderer::getInstanceExtensions();

    // Minimize memory allocation as much as possible
    auto extensions = std::vector<const char*>{};
    extensions.reserve(glfwExtensionCount + parentExtensions.size());

    extensions.insert(extensions.begin(), glfwExtensions, glfwExtensions + glfwExtensionCount);
    logExtensions("Instance", "tpd::PresentRenderer", extensions);

    extensions.insert(extensions.end(), std::make_move_iterator(parentExtensions.begin()), std::make_move_iterator(parentExtensions.end()));
    return extensions;
}

void tpd::StandardRenderer::framebufferResizeCallback(GLFWwindow* window, [[maybe_unused]] int width, [[maybe_unused]] int height) {
    const auto renderer = static_cast<StandardRenderer*>(glfwGetWindowUserPointer(window));
    renderer->_framebufferResized = true;
}

vk::Extent2D tpd::StandardRenderer::getFramebufferSize() const {
    if (!_engineInitialized) [[unlikely]] {
        PLOGW << "tpd::StandardRenderer - Requesting framebuffer size from a Renderer without any associated Engine: "
                 "the framebuffer size is undefined and should only be queried after the Engine::init(Renderer) call";
    }
    return _swapChainImageExtent;
}

void tpd::StandardRenderer::createSurface() {
    if (glfwCreateWindowSurface(_instance, _context->_window, nullptr, reinterpret_cast<VkSurfaceKHR*>(&_surface)) != VK_SUCCESS) [[unlikely]] {
        throw std::runtime_error("PresentRenderer - Failed to create a Vulkan surface");
    }
}

std::vector<const char*> tpd::StandardRenderer::getDeviceExtensions() const {
    // We must also append extensions required by parents, if any
    auto parentExtensions = Renderer::getDeviceExtensions();
    auto extensions = std::vector<const char*>{};
    extensions.reserve(parentExtensions.size() + 1);

    // A present renderer must be able to display rendered images
    extensions.push_back(vk::KHRSwapchainExtensionName);
    logExtensions("Device", "tpd::PresentRenderer", extensions);

    extensions.insert(extensions.end(), std::make_move_iterator(parentExtensions.begin()), std::make_move_iterator(parentExtensions.end()));
    return extensions;
}

void tpd::StandardRenderer::engineInit(const vk::Device device, const PhysicalDeviceSelection& physicalDeviceSelection) {
    // Clean up previously initialized swap chain resources before changing to new devices
    resetEngine();

    // Use parent to set new devices then update relevant queue information
    Renderer::engineInit(device, physicalDeviceSelection);

    // Just because graphics and present almost always come from the same queue family in practice, the Vulkan spec
    // doesn't impose such an assumption. We thus must account for the fact that these queues are distinct.
    _graphicsFamilyIndex = physicalDeviceSelection.graphicsQueueFamilyIndex;
    _presentFamilyIndex  = physicalDeviceSelection.presentQueueFamilyIndex;
    _graphicsQueue = _device.getQueue(physicalDeviceSelection.graphicsQueueFamilyIndex, 0);
    _presentQueue  = _device.getQueue(physicalDeviceSelection.presentQueueFamilyIndex,  0);

    // Create new swap chain resources
    createSwapChain();
    createSwapChainImageViews();

    // Create frame sync objects
    createSyncPrimitives();
}

void tpd::StandardRenderer::createSwapChain() {
    int width, height;
    glfwGetFramebufferSize(_context->_window, &width, &height);

    const auto swapChain = SwapChainBuilder()
        .desiredSurfaceFormat(vk::Format::eR8G8B8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear)
        .desiredPresentMode(vk::PresentModeKHR::eMailbox)
        .desiredExtent(static_cast<uint32_t>(width), static_cast<uint32_t>(height))
        .imageUsageFlags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst)
        .queueFamilyIndices(_graphicsFamilyIndex, _presentFamilyIndex)
        .build(_surface, _physicalDevice, _device);

    _swapChain = swapChain.swapChain;
    _swapChainImages = _device.getSwapchainImagesKHR(_swapChain);

    _swapChainImageFormat = swapChain.surfaceFormat.format;
    _swapChainImageExtent = swapChain.extent;

    PLOGD << "Swap chain created for tpd::PresentRenderer with:";
    PLOGD << " - Images count: " << _swapChainImages.size();
    PLOGD << " - Present mode: " << bootstrap::toString(swapChain.presentMode);
    PLOGD << " - Image extent: " << bootstrap::toString(_swapChainImageExtent);

#ifndef NDEBUG
    for (auto i = 0; i < _swapChainImages.size(); ++i) {
        bootstrap::setVulkanObjectName(
            static_cast<VkImage>(_swapChainImages[i]), vk::ObjectType::eImage,
            "tpd::StandardRenderer - SwapChain Image " + std::to_string(i),
            _instance, _device);
    }
#endif
}

void tpd::StandardRenderer::createSwapChainImageViews() {
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

    _swapChainImageViews = _swapChainImages | std::views::transform(toImageView) | std::ranges::to<std::vector>();

#ifndef NDEBUG
    for (auto i = 0; i < _swapChainImageViews.size(); ++i) {
        bootstrap::setVulkanObjectName(
            static_cast<VkImageView>(_swapChainImageViews[i]), vk::ObjectType::eImageView,
            "tpd::StandardRenderer - SwapChain ImageView " + std::to_string(i),
            _instance, _device);
    }
#endif
}

void tpd::StandardRenderer::createSyncPrimitives() {
    for (auto& [imageReady, renderDone, frameDrawFence] : _syncs) {
        imageReady = _device.createSemaphore({});
        renderDone = _device.createSemaphore({});
        frameDrawFence = _device.createFence({ vk::FenceCreateFlagBits::eSignaled });
    }
}

tpd::StandardRenderer::Presentable tpd::StandardRenderer::beginFrame() {
    auto frameData = Presentable{};

    using limits = std::numeric_limits<uint64_t>;
    [[maybe_unused]] const auto result = _device.waitForFences(_syncs[_currentFrame].frameDrawFence, vk::True, limits::max());

    uint32_t imageIndex;
    if (!acquireSwapChainImage(_syncs[_currentFrame].imageReady, &imageIndex)) [[unlikely]] {
        frameData.valid = false;
        return frameData;
    }

    // Only reset the fence if we're about to draw (by an Engine)
    _device.resetFences(_syncs[_currentFrame].frameDrawFence);

    frameData.valid = true;
    frameData.image = _swapChainImages[imageIndex];
    frameData.imageIndex = imageIndex;
    return frameData;
}

bool tpd::StandardRenderer::acquireSwapChainImage(const vk::Semaphore semaphore, uint32_t* imageIndex) {
    using limits = std::numeric_limits<uint64_t>;
    const auto result = _device.acquireNextImageKHR(_swapChain, limits::max(), semaphore, nullptr);

    if (result.result == vk::Result::eErrorOutOfDateKHR) [[unlikely]] {
        refreshSwapChain();
        return false;
    }
    if (result.result != vk::Result::eSuccess && result.result != vk::Result::eSuboptimalKHR) [[unlikely]] {
        throw std::runtime_error("PresentRenderer - Failed to acquire a swap chain image");
    }

    *imageIndex = result.value;
    return true;
}

void tpd::StandardRenderer::endFrame(const vk::ArrayProxy<vk::CommandBuffer>& buffers, const uint32_t imageIndex) {
    const auto bufferInfos = buffers
        | std::views::transform([](const auto buffer) { return vk::CommandBufferSubmitInfo{ buffer, 0 }; })
        | std::ranges::to<std::vector>();

    auto waitInfo = vk::SemaphoreSubmitInfo{};
    waitInfo.semaphore = _syncs[_currentFrame].imageReady;
    waitInfo.stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
    waitInfo.deviceIndex = 0;  // synchronize across GPUs
    waitInfo.value = 1;        // for timeline semaphores

    auto signalInfo = vk::SemaphoreSubmitInfo{};
    signalInfo.semaphore = _syncs[_currentFrame].renderDone;
    signalInfo.stageMask = vk::PipelineStageFlagBits2::eAllGraphics;  // TODO: find a more optimal mask
    signalInfo.deviceIndex = 0;  // synchronize across GPUs
    signalInfo.value = 1;        // for timeline semaphores

    auto submitInfo = vk::SubmitInfo2{};
    submitInfo.commandBufferInfoCount = bufferInfos.size();
    submitInfo.pCommandBufferInfos = bufferInfos.data();
    submitInfo.waitSemaphoreInfoCount = 1;
    submitInfo.pWaitSemaphoreInfos = &waitInfo;
    submitInfo.signalSemaphoreInfoCount = 1;
    submitInfo.pSignalSemaphoreInfos = &signalInfo;

    _graphicsQueue.submit2(submitInfo, _syncs[_currentFrame].frameDrawFence);
    presentSwapChainImage(imageIndex, _syncs[_currentFrame].renderDone);

    _currentFrame = (_currentFrame + 1) % IN_FLIGHT_FRAMES;
}

void tpd::StandardRenderer::presentSwapChainImage(const uint32_t imageIndex, const vk::Semaphore semaphore) {
    const auto presentInfo = vk::PresentInfoKHR{ semaphore, _swapChain, imageIndex };

    if (const auto result = _graphicsQueue.presentKHR(&presentInfo);
        result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || _framebufferResized) [[unlikely]] {
        _framebufferResized = false;
        refreshSwapChain();
    } else if (result != vk::Result::eSuccess) [[unlikely]] {
        throw std::runtime_error("PresentRenderer - Failed to present a swap chain image");
    }
}

void tpd::StandardRenderer::refreshSwapChain() {
    // We're not refreshing while the window is being minimized
    auto width = 0, height = 0;
    glfwGetFramebufferSize(_context->_window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(_context->_window, &width, &height);
        glfwWaitEvents();
    }

    // Before cleaning, we shouldn’t touch resources that may still be in use
    _device.waitIdle();
    cleanupSwapChain();

    // Recreate new swap chain and resources
    createSwapChain();
    createSwapChainImageViews();
}

void tpd::StandardRenderer::cleanupSwapChain() const noexcept {
    std::ranges::for_each(_swapChainImageViews, [this](const auto& it) { _device.destroyImageView(it); });
    _device.destroySwapchainKHR(_swapChain);
}

void tpd::StandardRenderer::resetEngine() noexcept {
    if (_engineInitialized) {
        // Make sure the GPU has stopped doing its things
        _device.waitIdle();

        // Destroy all frame resources
        std::ranges::for_each(_syncs, [this](const auto& frame) {
            _device.destroyFence(frame.frameDrawFence);
            _device.destroySemaphore(frame.renderDone);
            _device.destroySemaphore(frame.imageReady);
        });

        // Destroy all swap chain resources
        cleanupSwapChain();
    }
    Renderer::resetEngine();
}

void tpd::StandardRenderer::reset() noexcept {
    resetEngine();
    if (_initialized) {
        _instance.destroySurfaceKHR(_surface);
        _context.reset();  // tear down GLFW
    }
    Renderer::reset();
}

tpd::StandardRenderer::~StandardRenderer() noexcept {
    reset();
}
