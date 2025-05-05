#include "torpedo/rendering/SurfaceRenderer.h"
#include "torpedo/rendering/LogUtils.h"

#include <torpedo/bootstrap/SwapChainBuilder.h>
#include <torpedo/bootstrap/DebugUtils.h>

#include <torpedo/foundation/Image.h>
#include <torpedo/foundation/ImageUtils.h>

#include <chrono>

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
    utils::logDebugExtensions("Instance", "tpd::SurfaceRenderer", extensions);

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
    utils::logDebugExtensions("Device", "tpd::SurfaceRenderer", extensions);

    extensions.insert(extensions.end(), std::make_move_iterator(parentExtensions.begin()), std::make_move_iterator(parentExtensions.end()));
    return extensions;
}

void tpd::SurfaceRenderer::init(const uint32_t frameWidth, const uint32_t frameHeight) {
    if (initialized()) [[unlikely]] {
        PLOGI << "Skipping already initialized renderer: tpd::SurfaceRenderer";
        return;
    }

    // Creating a GLFW window
    _window = std::make_unique<Window>(vk::Extent2D{ frameWidth, frameHeight });
    glfwSetWindowUserPointer(_window->_glfwWindow, this);
    glfwSetFramebufferSizeCallback(_window->_glfwWindow, framebufferResizeCallback);

    // Bind the created window to the Vulkan surface
    createSurface();

    PLOGI << "Initialized renderer: tpd::SurfaceRenderer";
    PLOGD << "Number of in-flight frames run by tpd::SurfaceRenderer: " << getInFlightFrameCount();
}

void tpd::SurfaceRenderer::init(const bool fullscreen) {
    if (initialized()) [[unlikely]] {
        PLOGI << "Skipping already initialized renderer: tpd::SurfaceRenderer";
        return;
    }

    PLOGI << "Initializing renderer: tpd::SurfaceRenderer";
    _window = std::make_unique<Window>(fullscreen);
    glfwSetWindowUserPointer(_window->_glfwWindow, this);
    glfwSetFramebufferSizeCallback(_window->_glfwWindow, framebufferResizeCallback);

    // Bind the created window to the Vulkan surface
    createSurface();

    PLOGI << "Initialized renderer: tpd::SurfaceRenderer";
    PLOGD << "Number of in-flight frames run by tpd::SurfaceRenderer: " << getInFlightFrameCount();
}

void tpd::SurfaceRenderer::framebufferResizeCallback(GLFWwindow* window, [[maybe_unused]] int width, [[maybe_unused]] int height) {
    const auto renderer = static_cast<SurfaceRenderer*>(glfwGetWindowUserPointer(window));
    renderer->_framebufferResized = true;
}

void tpd::SurfaceRenderer::createSurface() {
    if (glfwCreateWindowSurface(_instance, _window->_glfwWindow, nullptr, reinterpret_cast<VkSurfaceKHR*>(&_surface)) != VK_SUCCESS) [[unlikely]] {
        throw std::runtime_error("SurfaceRenderer - Failed to create a Vulkan surface");
    }
}

void tpd::SurfaceRenderer::engineInit(const uint32_t graphicsFamily, const uint32_t presentFamily, std::pmr::memory_resource* frameResource) {
    _graphicsFamilyIndex = graphicsFamily;
    _presentFamilyIndex = presentFamily;
    _presentQueue  = _device.getQueue(_presentFamilyIndex,  0);

    createSwapChain();
    createFrameSyncPrimitives(frameResource);
}

void tpd::SurfaceRenderer::createSwapChain() {
    int width, height;
    glfwGetFramebufferSize(_window->_glfwWindow, &width, &height);

    const auto [swapChain, surfaceFormat, presentMode, extent, minImageCount] = SwapChainBuilder()
        .desiredSurfaceFormat(vk::Format::eR8G8B8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear)
        .desiredPresentMode(vk::PresentModeKHR::eMailbox)
        .desiredExtent(static_cast<uint32_t>(width), static_cast<uint32_t>(height))
        .imageUsageFlags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst)
        .queueFamilyIndices(_graphicsFamilyIndex, _presentFamilyIndex)
        .build(_surface, _physicalDevice, _device);

    _swapChain = swapChain;
    _swapChainImageExtent = extent;

    const auto swapImages = _device.getSwapchainImagesKHR(_swapChain);
    const auto swapImageCount = swapImages.size() > MAX_SWAP_IMAGES ? MAX_SWAP_IMAGES : swapImages.size();
    for (uint32_t i = 0; i < swapImageCount; i++) {
        _swapChainImages[i] = swapImages[i];
    }

    PLOGD << "Swap chain created for tpd::SurfaceRenderer with:";
    PLOGD << " - Present mode: " << utils::toString(presentMode);
    PLOGD << " - Image extent: " << utils::toString(_swapChainImageExtent);
    PLOGD << " - Image format: " << utils::toString(surfaceFormat.format);
    PLOGD << " - Color space:  " << utils::toString(surfaceFormat.colorSpace);
    PLOGD << " - Image count:  " << swapImageCount;

#ifndef NDEBUG
    for (auto i = 0; i < swapImageCount; ++i) {
        utils::setVulkanObjectName(
            static_cast<VkImage>(_swapChainImages[i]), vk::ObjectType::eImage,
            "tpd::SurfaceRenderer - SwapChain Image " + std::to_string(i),
            _instance, _device);
    }
#endif
}

void tpd::SurfaceRenderer::createFrameSyncPrimitives(std::pmr::memory_resource* frameResource) {
    _frameSyncs = std::pmr::vector<FrameSync>{ frameResource };
    _frameSyncs.reserve(IN_FLIGHT_FRAME_COUNT);
    _frameSyncs.resize(IN_FLIGHT_FRAME_COUNT);

    for (auto& [imageReady, renderDone, frameDrawFence] : _frameSyncs) {
        imageReady = _device.createSemaphore({});
        renderDone = _device.createSemaphore({});
        frameDrawFence = _device.createFence({ vk::FenceCreateFlagBits::eSignaled });
    }
}

tpd::SwapImage tpd::SurfaceRenderer::launchFrame() {
    using limits = std::numeric_limits<uint64_t>;
    [[maybe_unused]] const auto result = _device.waitForFences(_frameSyncs[_currentFrame].frameDrawFence, vk::True, limits::max());

    uint32_t imageIndex;
    if (!acquireSwapChainImage(_frameSyncs[_currentFrame].imageReady, &imageIndex)) [[unlikely]] {
        return {}; // tell call site to skip this frame
    }

    // Only reset the fence if we're about to draw (by an Engine)
    _device.resetFences(_frameSyncs[_currentFrame].frameDrawFence);

    // The frame is valid, return the swap image and have the call site decide its contents
    return { _swapChainImages[imageIndex], imageIndex };
}

bool tpd::SurfaceRenderer::acquireSwapChainImage(const vk::Semaphore semaphore, uint32_t* imageIndex) {
    // We don't use Vulkan-Hpp here to acquire image since the hpp-function aggressively throws
    // when the result is not VK_SUCCESS; for swap chain image acquire, this is not always an error.
    const auto acquireInfo = VkAcquireNextImageInfoKHR{
        .sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR,
        .swapchain = _swapChain,
        .timeout = std::numeric_limits<uint64_t>::max(),
        .semaphore = semaphore,
        .deviceMask = 0b1, // ignored by single-GPU setups
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

void tpd::SurfaceRenderer::submitFrame(const uint32_t imageIndex) {
    presentSwapChainImage(imageIndex, _frameSyncs[_currentFrame].renderDone);
    _currentFrame = (_currentFrame + 1) % IN_FLIGHT_FRAME_COUNT;
}

void tpd::SurfaceRenderer::presentSwapChainImage(const uint32_t imageIndex, const vk::Semaphore renderDone) {
    // Similar to image acquire, presenting an image cannot be done with Vulkan-Hpp
    const auto waitSemaphore = static_cast<VkSemaphore>(renderDone);
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
        std::ranges::for_each(_frameSyncs, [this](const auto& frame) {
            _device.destroyFence(frame.frameDrawFence);
            _device.destroySemaphore(frame.renderDone);
            _device.destroySemaphore(frame.imageReady);
        });
        _frameSyncs.clear();

        // Destroy all swap chain resources
        cleanupSwapChain();
    }
    Renderer::resetEngine();
}

void tpd::SurfaceRenderer::destroy() noexcept {
    resetEngine();
    if (initialized()) {
        _instance.destroySurfaceKHR(_surface);
        _window.reset();
    }
    Renderer::destroy();
}

tpd::SurfaceRenderer::~SurfaceRenderer() noexcept {
    destroy();
}
