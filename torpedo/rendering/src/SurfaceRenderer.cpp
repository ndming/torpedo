#include "torpedo/rendering/SurfaceRenderer.h"
#include "torpedo/rendering/LogUtils.h"

#include <torpedo/bootstrap/SwapChainBuilder.h>
#include <torpedo/bootstrap/DebugUtils.h>
#include <torpedo/foundation/AllocationUtils.h>
#include <torpedo/foundation/ImageUtils.h>

#include <chrono>
#include <ranges>

tpd::SurfaceRenderer::Window::Window(const vk::Extent2D initialFramebufferSize) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    const auto w = static_cast<int>(initialFramebufferSize.width);
    const auto h = static_cast<int>(initialFramebufferSize.height);
    _glfwWindow = glfwCreateWindow(w, h, "torpedo", nullptr, nullptr);

    if (!_glfwWindow) {
        throw std::runtime_error("SurfaceRenderer::Window - Failed to create a GLFW window");
    }
}

tpd::SurfaceRenderer::Window::Window(const bool fullscreen) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    if (fullscreen) {
        const auto monitor = glfwGetPrimaryMonitor();
        if (!monitor) {
            throw std::runtime_error("SurfaceRenderer::Window - Failed to get a primary monitor");
        }
        const auto mode = glfwGetVideoMode(monitor);
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
        _glfwWindow = glfwCreateWindow(mode->width, mode->height, "torpedo", monitor, nullptr);
    } else {
        _glfwWindow = glfwCreateWindow(1280, 720, "torpedo", nullptr, nullptr);
        glfwMaximizeWindow(_glfwWindow);
    }

    if (!_glfwWindow) {
        throw std::runtime_error("SurfaceRenderer::Window - Failed to create a GLFW window");
    }
}

void tpd::SurfaceRenderer::Window::loop(const std::function<void()>& onRender) const {
    while (!glfwWindowShouldClose(_glfwWindow)) [[likely]] {
        glfwPollEvents();
        onRender();
    }
}

void tpd::SurfaceRenderer::Window::loop(const std::function<void(float)>& onRender) const {
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (!glfwWindowShouldClose(_glfwWindow)) [[likely]] {
        glfwPollEvents();

        const auto currentTime = std::chrono::high_resolution_clock::now();
        const float deltaTimeMillis = std::chrono::duration<float, std::milli>(currentTime - lastTime).count();
        onRender(deltaTimeMillis);

        lastTime = currentTime;
    }
}

tpd::SurfaceRenderer::Window::~Window() {
    glfwDestroyWindow(_glfwWindow);
    _glfwWindow = nullptr;
    glfwTerminate();
}

#if defined(_WIN32)
static constexpr auto surfaceExtensions = std::array {
    vk::KHRSurfaceExtensionName,
    "VK_KHR_win32_surface",
};
#elif defined(__linux__)
#if defined(WAYLAND)
static constexpr auto surfaceExtensions = std::array {
    vk::KHRSurfaceExtensionName,
    "VK_KHR_wayland_surface",
};
#else
static constexpr auto surfaceExtensions = std::array {
    vk::KHRSurfaceExtensionName,
    "VK_KHR_xcb_surface",
    "VK_KHR_xlib_surface",
};
#endif
#elif defined(__APPLE__)
static constexpr auto surfaceExtensions = std::array {
    vk::KHRSurfaceExtensionName,
    "VK_MVK_macos_surface",
};
#elif defined(__ANDROID__)
static constexpr auto surfaceExtensions = std::array {
    vk::KHRSurfaceExtensionName,
    "VK_KHR_android_surface",
};
#else
PLOGI << "Detected no surface capability support, how about working with tpd::HeadlessRenderer?";
static constexpr auto surfaceExtensions = std::array<const char*, 0>{};
#endif

std::vector<const char*> tpd::SurfaceRenderer::getInstanceExtensions() const {
    // Override to add extensions supporting surface representation
    // We must also append extensions from parents, if any
    auto parentExtensions = Renderer::getInstanceExtensions();

    // Minimize memory allocation as much as possible
    auto extensions = std::vector<const char*>{};
    extensions.reserve(surfaceExtensions.size() + parentExtensions.size());

    extensions.insert(extensions.begin(), surfaceExtensions.begin(), surfaceExtensions.end());
    rendering::logDebugExtensions("Instance", "tpd::SurfaceRenderer", extensions);

    extensions.insert(extensions.end(), std::make_move_iterator(parentExtensions.begin()), std::make_move_iterator(parentExtensions.end()));
    return extensions;
}

std::vector<const char*> tpd::SurfaceRenderer::getDeviceExtensions() const {
    // We must also append extensions required by parents, if any
    auto parentExtensions = Renderer::getDeviceExtensions();
    auto extensions = std::vector<const char*>{};
    extensions.reserve(parentExtensions.size() + 1);

    // A present renderer must be able to display rendered images
    extensions.push_back(vk::KHRSwapchainExtensionName);
    rendering::logDebugExtensions("Device", "tpd::SurfaceRenderer", extensions);

    extensions.insert(extensions.end(), std::make_move_iterator(parentExtensions.begin()), std::make_move_iterator(parentExtensions.end()));
    return extensions;
}

void tpd::SurfaceRenderer::init(const uint32_t frameWidth, const uint32_t frameHeight, std::pmr::memory_resource* contextPool) {
    if (_initialized) [[unlikely]] {
        PLOGI << "Skipping already initialized renderer: tpd::SurfaceRenderer";
        return;
    }

    // Creating a GLFW window
    _window = foundation::make_unique<Window>(contextPool, vk::Extent2D{ frameWidth, frameHeight });
    glfwSetWindowUserPointer(_window->_glfwWindow, this);
    glfwSetFramebufferSizeCallback(_window->_glfwWindow, framebufferResizeCallback);

    // Bind the created window to the Vulkan surface
    createSurface();

    PLOGI << "Initialized renderer: tpd::SurfaceRenderer";
    PLOGD << "Number of in-flight frames run by tpd::SurfaceRenderer: " << getInFlightFramesCount();
}

void tpd::SurfaceRenderer::init(const bool fullscreen, std::pmr::memory_resource* contextPool) {
    if (_initialized) [[unlikely]] {
        PLOGI << "Skipping already initialized renderer: tpd::SurfaceRenderer";
        return;
    }

    PLOGI << "Initializing renderer: tpd::SurfaceRenderer";
    _window = foundation::make_unique<Window>(contextPool, fullscreen);
    glfwSetWindowUserPointer(_window->_glfwWindow, this);
    glfwSetFramebufferSizeCallback(_window->_glfwWindow, framebufferResizeCallback);

    // Bind the created window to the Vulkan surface
    createSurface();

    PLOGI << "Initialized renderer: tpd::SurfaceRenderer";
    PLOGD << "Number of in-flight frames run by tpd::SurfaceRenderer: " << getInFlightFramesCount();
}

void tpd::SurfaceRenderer::framebufferResizeCallback(GLFWwindow* window, [[maybe_unused]] int width, [[maybe_unused]] int height) {
    const auto renderer = static_cast<SurfaceRenderer*>(glfwGetWindowUserPointer(window));
    renderer->_framebufferResized = true;
}

vk::Extent2D tpd::SurfaceRenderer::getFramebufferSize() const noexcept {
    if (!_engineInitialized) [[unlikely]] {
        PLOGW << "tpd::SurfaceRenderer - Requesting framebuffer size from a Renderer associated with an unbound Context: "
                 "the framebuffer size is undefined and should only be queried after the Context::bindEngine() call";
    }
    return _swapChainImageExtent;
}

void tpd::SurfaceRenderer::createSurface() {
    if (glfwCreateWindowSurface(_instance, _window->_glfwWindow, nullptr, reinterpret_cast<VkSurfaceKHR*>(&_surface)) != VK_SUCCESS) [[unlikely]] {
        throw std::runtime_error("SurfaceRenderer - Failed to create a Vulkan surface");
    }
}

void tpd::SurfaceRenderer::engineInit(const uint32_t graphicsFamilyIndex, const uint32_t presentFamilyIndex) {
    // Just because graphics and present almost always come from the same queue family in practice, the Vulkan spec
    // doesn't impose such an assumption. We thus must account for the fact that these queues might be distinct.
    _graphicsFamilyIndex = graphicsFamilyIndex;
    _presentFamilyIndex  = presentFamilyIndex;
    _graphicsQueue = _device.getQueue(_graphicsFamilyIndex, 0);
    _presentQueue  = _device.getQueue(_presentFamilyIndex,  0);

    createSwapChain();
    createSyncPrimitives();
}

void tpd::SurfaceRenderer::createSwapChain() {
    int width, height;
    glfwGetFramebufferSize(_window->_glfwWindow, &width, &height);

    if (_graphicsFamilyIndex != _presentFamilyIndex) {
        PLOGW << "tpd::SurfaceRenderer - Graphics and Present queues are distinct: "
                 "this will likely lead to suboptimal performance in some cases";
    }

    const auto [swapChain, surfaceFormat, presentMode, extent, minImageCount] = SwapChainBuilder()
        .desiredSurfaceFormat(vk::Format::eR8G8B8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear)
        .desiredPresentMode(vk::PresentModeKHR::eMailbox)
        .desiredExtent(static_cast<uint32_t>(width), static_cast<uint32_t>(height))
        .imageUsageFlags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst)
        .queueFamilyIndices(_graphicsFamilyIndex, _presentFamilyIndex)
        .build(_surface, _physicalDevice, _device);

    _swapChain = swapChain;
    _swapChainImageFormat = surfaceFormat.format;
    _swapChainImageExtent = extent;

    const auto swapImages = _device.getSwapchainImagesKHR(_swapChain);
    const auto swapImageCount = swapImages.size() > MAX_SWAP_IMAGES ? MAX_SWAP_IMAGES : swapImages.size();
    for (uint32_t i = 0; i < swapImageCount; i++) {
        _swapChainImages[i] = swapImages[i];
    }

    PLOGD << "Swap chain created for tpd::SurfaceRenderer with:";
    PLOGD << " - Present mode: " << rendering::toString(presentMode);
    PLOGD << " - Image extent: " << rendering::toString(_swapChainImageExtent);
    PLOGD << " - Image format: " << foundation::toString(_swapChainImageFormat);
    PLOGD << " - Color space:  " << rendering::toString(surfaceFormat.colorSpace);
    PLOGD << " - Image count:  " << swapImageCount;

#ifndef NDEBUG
    for (auto i = 0; i < swapImageCount; ++i) {
        bootstrap::setVulkanObjectName(
            static_cast<VkImage>(_swapChainImages[i]), vk::ObjectType::eImage,
            "tpd::SurfaceRenderer - SwapChain Image " + std::to_string(i),
            _instance, _device);
    }
#endif
}

void tpd::SurfaceRenderer::createSyncPrimitives() {
    for (auto& [imageReady, renderDone, frameDrawFence] : _syncs) {
        imageReady = _device.createSemaphore({});
        renderDone = _device.createSemaphore({});
        frameDrawFence = _device.createFence({ vk::FenceCreateFlagBits::eSignaled });
    }
}

tpd::SurfaceRenderer::Presentable tpd::SurfaceRenderer::launchFrame() {
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

bool tpd::SurfaceRenderer::acquireSwapChainImage(const vk::Semaphore semaphore, uint32_t* imageIndex) {
    // We don't use Vulkan-Hpp here to acquire image since the hpp-function aggressively throws
    // when the result is not VK_SUCCESS; for swap chain image acquire, this is not always an error.
    const auto acquireInfo = VkAcquireNextImageInfoKHR{
        .sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR,
        .swapchain = _swapChain,
        .timeout = std::numeric_limits<uint64_t>::max(),
        .semaphore = semaphore,
        .deviceMask = 0b1,  // ignored by single-GPU setups
    };
    const auto result = vkAcquireNextImage2KHR(_device, &acquireInfo, imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) [[unlikely]] {
        refreshSwapChain();
        return false;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) [[unlikely]] {
        throw std::runtime_error("SurfaceRenderer - Failed to acquire a swap chain image");
    }

    return true;
}

void tpd::SurfaceRenderer::submitFrame(
    const uint32_t imageIndex,
    const vk::CommandBuffer buffer,
    const vk::PipelineStageFlags2 waitStage,
    const vk::PipelineStageFlags2 doneStage,
    const std::vector<std::pair<vk::Semaphore, vk::PipelineStageFlags2>>& waitSemaphores)
{
    auto bufferInfo = vk::CommandBufferSubmitInfo{};
    bufferInfo.commandBuffer = buffer;
    bufferInfo.deviceMask    = 0b1;

    auto waitInfos = std::vector<vk::SemaphoreSubmitInfo>{};
    waitInfos.reserve(waitSemaphores.size() + 1);
    waitInfos.emplace_back(_syncs[_currentFrame].imageReady, 1, waitStage, 0);
    for (const auto [semaphore, stage] : waitSemaphores) {
        waitInfos.emplace_back(semaphore, 1, stage, 0);
    }

    auto doneInfo = vk::SemaphoreSubmitInfo{};
    doneInfo.semaphore = _syncs[_currentFrame].renderDone;
    doneInfo.stageMask = doneStage;
    doneInfo.deviceIndex = 0;  // synchronize across GPUs
    doneInfo.value = 1;        // for timeline semaphores

    auto submitInfo = vk::SubmitInfo2{};
    submitInfo.commandBufferInfoCount = 1;
    submitInfo.pCommandBufferInfos = &bufferInfo;
    submitInfo.waitSemaphoreInfoCount = static_cast<uint32_t>(waitInfos.size());
    submitInfo.pWaitSemaphoreInfos = waitInfos.data();
    submitInfo.signalSemaphoreInfoCount = 1;
    submitInfo.pSignalSemaphoreInfos = &doneInfo;

    _graphicsQueue.submit2(submitInfo, _syncs[_currentFrame].frameDrawFence);
    presentSwapChainImage(imageIndex, _syncs[_currentFrame].renderDone);

    _currentFrame = (_currentFrame + 1) % IN_FLIGHT_FRAMES;
}

void tpd::SurfaceRenderer::presentSwapChainImage(const uint32_t imageIndex, const vk::Semaphore semaphore) {
    // Similar to image acquire, presenting an image cannot be done with Vulkan-Hpp
    const auto waitSemaphore = static_cast<VkSemaphore>(semaphore);
    const auto swapChain = static_cast<VkSwapchainKHR>(_swapChain);

    const auto presentInfo = VkPresentInfoKHR{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &waitSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &swapChain,
        .pImageIndices = &imageIndex,
    };

    if (const auto result = vkQueuePresentKHR(_presentQueue, &presentInfo);
        result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _framebufferResized) [[unlikely]] {
        _framebufferResized = false;
        refreshSwapChain();
    } else if (result != VK_SUCCESS) [[unlikely]] {
        throw std::runtime_error("SurfaceRenderer - Failed to present a swap chain image");
    }
}

void tpd::SurfaceRenderer::refreshSwapChain() {
    // We're not refreshing while the window is being minimized
    auto width = 0, height = 0;
    glfwGetFramebufferSize(_window->_glfwWindow, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(_window->_glfwWindow, &width, &height);
        glfwWaitEvents();
    }

    // Before cleaning, we shouldn’t touch resources that may still be in use
    _device.waitIdle();
    cleanupSwapChain();

    // Recreate new swap chain and resources
    createSwapChain();

    // Notify all listeners about the change
    for (const auto& [ptr, callback] : _framebufferResizeListeners) {
        callback(ptr, width, height);
    }
}

void tpd::SurfaceRenderer::cleanupSwapChain() const noexcept {
    _device.destroySwapchainKHR(_swapChain);
}

void tpd::SurfaceRenderer::resetEngine() noexcept {
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

void tpd::SurfaceRenderer::destroy() noexcept {
    resetEngine();
    if (_initialized) {
        _instance.destroySurfaceKHR(_surface);
        _window.reset();
    }
    Renderer::destroy();
}

tpd::SurfaceRenderer::~SurfaceRenderer() noexcept {
    destroy();
}
