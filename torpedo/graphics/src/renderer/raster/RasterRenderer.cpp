#include "renderer/raster/RasterRenderer.h"

#include <torpedo/bootstrap/PhysicalDeviceSelector.h>
#include <torpedo/bootstrap/DeviceBuilder.h>

#include <plog/Log.h>

#include <ranges>

tpd::renderer::RasterRenderer::RasterRenderer(GLFWwindow* const window) : Renderer{}, _window{ window } {
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void tpd::renderer::RasterRenderer::framebufferResizeCallback(
    GLFWwindow* window,
    [[maybe_unused]] const int width,
    [[maybe_unused]] const int height)
{
    const auto renderer = static_cast<RasterRenderer*>(glfwGetWindowUserPointer(window));
    renderer->_framebufferResized = true;
}

// =====================================================================================================================
// INITIALIZATIONS
// =====================================================================================================================

void tpd::renderer::RasterRenderer::onCreate(
    const vk::Instance instance,
    const std::initializer_list<const char*> deviceExtensions)
{
    // We want the surface to be available first since we will override the pickPhysicalDevice function to request
    // a present queue which, in turn, depends on the surface
    createSurface(instance);
    Renderer::onCreate(instance, deviceExtensions);
}

void tpd::renderer::RasterRenderer::onInitialize() {
    Renderer::onInitialize();

    // Initialize swap chain specific resources
    createSwapChain();
    createSwapChainImageViews();

    // Initialize framebuffer resources and render pass
    createFramebufferResources();
    createRenderPass();
    createFramebuffers();

    // Initialize drawing-specific resources
    createDrawingCommandBuffers();
    createDrawingSyncObjects();
}

void tpd::renderer::RasterRenderer::createSurface(const vk::Instance instance) {
    if (glfwCreateWindowSurface(instance, _window, nullptr, reinterpret_cast<VkSurfaceKHR*>(&_surface)) != VK_SUCCESS) {
        throw std::runtime_error("StandardRenderer: failed to create a Vulkan surface");
    }
}

void tpd::renderer::RasterRenderer::pickPhysicalDevice(
    const vk::Instance instance,
    const std::initializer_list<const char*> deviceExtensions)
{
    const auto selector = getPhysicalDeviceSelector(deviceExtensions).select(instance);

    _physicalDevice = selector.getPhysicalDevice();
    _graphicsQueueFamily = selector.getGraphicsQueueFamily();
    _presentQueueFamily  = selector.getPresentQueueFamily();

    PLOGD << "StandardRenderer - Found a suitable device: " << _physicalDevice.getProperties().deviceName.data();
    PLOGD << "StandardRenderer - Graphics queue family: " << _graphicsQueueFamily;
    PLOGD << "StandardRenderer - Present queue family:  " << _presentQueueFamily;
}

tpd::PhysicalDeviceSelector tpd::renderer::RasterRenderer::getPhysicalDeviceSelector(
    const std::initializer_list<const char*> deviceExtensions) const
{
    return Renderer::getPhysicalDeviceSelector(deviceExtensions)
        .requestPresentQueueFamily(_surface)
        .features(getFeatures())
        .featuresVertexInputDynamicState(getVertexInputDynamicStateFeatures())
        .featuresExtendedDynamicState(getExtendedDynamicStateFeatures())
        .featuresExtendedDynamicState3(getExtendedDynamicState3Features());
}

void tpd::renderer::RasterRenderer::createDevice(const std::initializer_list<const char*> deviceExtensions) {
    _device = DeviceBuilder()
        .deviceExtensions(getDeviceExtensions(deviceExtensions))
        .deviceFeatures(buildDeviceFeatures(getFeatures()))
        .queueFamilyIndices({ _graphicsQueueFamily, _presentQueueFamily })
        .build(_physicalDevice);

    _graphicsQueue = _device.getQueue(_graphicsQueueFamily, 0);
    _presentQueue  = _device.getQueue(_presentQueueFamily,  0);
}

std::vector<const char*> tpd::renderer::RasterRenderer::getRendererExtensions() const {
    auto rendererExtensions = Renderer::getRendererExtensions();
    // A raster renderer must be able to display rendered images
    rendererExtensions.push_back(vk::KHRSwapchainExtensionName);
    // For dynamic vertex bindings and attributes
    rendererExtensions.push_back(vk::EXTVertexInputDynamicStateExtensionName);
    // For dynamic polygon mode and rasterization samples
    rendererExtensions.push_back(vk::EXTExtendedDynamicState3ExtensionName);
    return rendererExtensions;
}

vk::PhysicalDeviceFeatures tpd::renderer::RasterRenderer::getFeatures() {
    auto features = vk::PhysicalDeviceFeatures{};
    features.largePoints       = vk::True;  // for custom gl_PointSize in vertex shader
    features.wideLines         = vk::True;  // for custom line width
    features.fillModeNonSolid  = vk::True;  // for custom polygon mode
    features.samplerAnisotropy = vk::True;  // required by raster materials
    features.sampleRateShading = vk::True;  // required by raster materials
    return features;
}

void tpd::renderer::RasterRenderer::onFeaturesRegister() {
    Renderer::onFeaturesRegister();
    // Dynamic vertex input state
    addFeature(getVertexInputDynamicStateFeatures());
    // Dynamic cull mode, front face, and primitive topology
    addFeature(getExtendedDynamicStateFeatures());
    // Dynamic polygon mode and MSAA
    addFeature(getExtendedDynamicState3Features());
}

vk::PhysicalDeviceVertexInputDynamicStateFeaturesEXT tpd::renderer::RasterRenderer::getVertexInputDynamicStateFeatures() {
    auto vertexInputDynamicStateFeatures = vk::PhysicalDeviceVertexInputDynamicStateFeaturesEXT{};
    vertexInputDynamicStateFeatures.vertexInputDynamicState = vk::True;
    return vertexInputDynamicStateFeatures;
}

vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT tpd::renderer::RasterRenderer::getExtendedDynamicStateFeatures() {
    auto extendedDynamicStateFeatures = vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT{};
    extendedDynamicStateFeatures.extendedDynamicState = vk::True;
    return extendedDynamicStateFeatures;
}

vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT tpd::renderer::RasterRenderer::getExtendedDynamicState3Features() {
    auto extendedDynamicState3Features = vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT{};
    extendedDynamicState3Features.extendedDynamicState3PolygonMode          = vk::True;
    extendedDynamicState3Features.extendedDynamicState3RasterizationSamples = vk::True;
    return extendedDynamicState3Features;
}

// =====================================================================================================================
// SWAP CHAIN INFRASTRUCTURE
// =====================================================================================================================

void tpd::renderer::RasterRenderer::createSwapChain() {
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
        const uint32_t queueFamilyIndices[]{ _graphicsQueueFamily, _presentQueueFamily };
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
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

vk::SurfaceFormatKHR tpd::renderer::RasterRenderer::chooseSwapSurfaceFormat() const {
    const auto availableFormats = _physicalDevice.getSurfaceFormatsKHR(_surface);

    constexpr auto format = vk::Format::eB8G8R8A8Srgb;
    constexpr auto colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;

    const auto suitable = [](const auto& it){ return it.format == format && it.colorSpace == colorSpace; };
    if (const auto found = std::ranges::find_if(availableFormats, suitable); found != availableFormats.end()) {
        return *found;
    }

    return availableFormats[0];
}

vk::PresentModeKHR tpd::renderer::RasterRenderer::chooseSwapPresentMode() const {
    const auto presentModes = _physicalDevice.getSurfacePresentModesKHR(_surface);

    constexpr auto preferredMode = vk::PresentModeKHR::eMailbox;
    if (const auto found = std::ranges::find(presentModes, preferredMode); found != presentModes.end()) {
        return preferredMode;
    }

    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D tpd::renderer::RasterRenderer::chooseSwapExtent() const {
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

// =====================================================================================================================
// SWAP CHAIN IMAGE VIEWS
// =====================================================================================================================

void tpd::renderer::RasterRenderer::createSwapChainImageViews() {
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
}

// =====================================================================================================================
// SWAP CHAIN UTILITIES
// =====================================================================================================================

void tpd::renderer::RasterRenderer::recreateSwapChain() {
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

    createFramebufferResources();
    createFramebuffers();
}

void tpd::renderer::RasterRenderer::cleanupSwapChain() const noexcept {
    // Destroy all framebuffers' attachments
    destroyFramebufferResources();

    // Destroy the framebuffers and swap chain image views
    using namespace std::ranges;
    for_each(_framebuffers, [this](const auto& it) { _device.destroyFramebuffer(it); });
    for_each(_swapChainImageViews, [this](const auto& it) { _device.destroyImageView(it); });

    // Get rid of the old swap chain
    _device.destroySwapchainKHR(_swapChain);
}

bool tpd::renderer::RasterRenderer::acquireSwapChainImage(const vk::Semaphore semaphore, uint32_t* imageIndex) {
    using limits = std::numeric_limits<uint64_t>;
    const auto result = _device.acquireNextImageKHR(_swapChain, limits::max(), semaphore, nullptr);

    if (result.result == vk::Result::eErrorOutOfDateKHR) {
        recreateSwapChain();
        return false;
    }
    if (result.result != vk::Result::eSuccess && result.result != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("StandardRenderer - Failed to acquire a swap chain image");
    }

    *imageIndex = result.value;
    return true;
}

void tpd::renderer::RasterRenderer::presentSwapChainImage(const uint32_t imageIndex, const vk::Semaphore semaphore) {
    const auto presentInfo = vk::PresentInfoKHR{ semaphore, _swapChain, imageIndex };

    if (const auto result = _presentQueue.presentKHR(&presentInfo);
        result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || _framebufferResized) {
        _framebufferResized = false;
        recreateSwapChain();
    } else if (result != vk::Result::eSuccess) {
        throw std::runtime_error("StandardRenderer - Failed to present a swap chain image");
    }
}

// =====================================================================================================================
// RENDER PASS & FRAMEBUFFERS
// =====================================================================================================================

void tpd::renderer::RasterRenderer::createRenderPass() {
    const auto colorAttachment = vk::AttachmentDescription{
        {}, _swapChainImageFormat,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,     // clear the framebuffer to black before drawing a new frame
        vk::AttachmentStoreOp::eStore,    // rendered contents will be stored in memory to be resolved later
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal
    };

    // We should keep the order we specify these attachments in mind because later on we will have to specify
    // clear values for them in the same order. Also, the attachment reference objects reference attachments using the
    // indices of this array
    const auto attachments = std::array{ colorAttachment };

    constexpr auto colorAttachmentRef = vk::AttachmentReference{ 0, vk::ImageLayout::eColorAttachmentOptimal };

    // Define a subpass for our rendering
    auto subpass = vk::SubpassDescription{};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    // The index of the attachment in this array is directly referenced from the fragment shader with the
    // "layout(location = <index>) out" directive
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    // Subpasses in a render pass automatically take care of image layout transitions. These transitions are controlled
    // by subpass dependencies, which specify memory and execution dependencies between subpasses. We have only a single
    // subpass right now, but the operations right before and right after this subpass also count as implicit "subpasses"
    // There are two built-in dependencies that take care of the transition at the start of the render pass and at the
    // end of the render pass, but the former does not occur at the right time. It assumes that the transition occurs
    // at the start of the pipeline, but we haven’t acquired the image yet at that point because the endFrame uses
    // eColorAttachmentOutput for the drawing wait stage. There are two ways to deal with this problem:
    // - In the endFrame method, change the waitStages for the imageAvailableSemaphore to eTopOfPipe to ensure that
    // the render passes don’t begin until the image is available
    // - Make the render pass wait for the eColorAttachmentOutput stage, using subpass dependency
    using Stage = vk::PipelineStageFlagBits;
    using Access = vk::AccessFlagBits;

    constexpr auto srcStages = Stage::eColorAttachmentOutput;
    constexpr auto dstStages = Stage::eColorAttachmentOutput;
    constexpr auto srcAccess = vk::AccessFlags{ 0 };
    constexpr auto dstAccess = Access::eColorAttachmentWrite;

    constexpr auto dependency = vk::SubpassDependency{
        vk::SubpassExternal,  // src subpass
        0,                    // dst subpass
        srcStages, dstStages,
        srcAccess, dstAccess,
    };

    const auto renderPassInfo = vk::RenderPassCreateInfo{ {}, attachments, subpass, dependency };
    _renderPass = _device.createRenderPass(renderPassInfo);
}

void tpd::renderer::RasterRenderer::createFramebuffers() {
    const auto toFramebuffer = [this](const auto& swapChainImageView) {
        // Specify the image views that should be bound to the respective attachment descriptions in the render pass
        const auto attachments = std::array{ swapChainImageView };

        auto framebufferInfo = vk::FramebufferCreateInfo{};
        framebufferInfo.renderPass = _renderPass;
        framebufferInfo.attachmentCount = attachments.size();
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = _swapChainImageExtent.width;
        framebufferInfo.height = _swapChainImageExtent.height;
        framebufferInfo.layers = 1;  // our swap chain images are single images, so the number of layers is 1

        return _device.createFramebuffer(framebufferInfo);
    };

    _framebuffers = _swapChainImageViews | std::ranges::views::transform(toFramebuffer) | std::ranges::to<std::vector>();
}

// =====================================================================================================================
// DRAWING RESOURCES
// =====================================================================================================================

void tpd::renderer::RasterRenderer::createDrawingCommandBuffers() {
    // Allocate a drawing command buffer for each in-flight frame
    auto allocInfo = vk::CommandBufferAllocateInfo{};
    allocInfo.commandPool = _drawingCommandPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

    auto commandBuffers = _device.allocateCommandBuffers(allocInfo);
    std::ranges::move(commandBuffers.begin(), commandBuffers.end(), _drawingCommandBuffers.begin());
}

void tpd::renderer::RasterRenderer::createDrawingSyncObjects() {
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        _imageAvailableSemaphores[i] = _device.createSemaphore({});
        _renderFinishedSemaphores[i] = _device.createSemaphore({});
        _drawingInFlightFences[i] = _device.createFence({ vk::FenceCreateFlagBits::eSignaled });
    }
}

void tpd::renderer::RasterRenderer::destroyDrawingSyncObjects() const noexcept {
    using namespace std::ranges;
    for_each(_drawingInFlightFences, [this](const auto& it) { _device.destroyFence(it); });
    for_each(_renderFinishedSemaphores, [this](const auto& it) { _device.destroySemaphore(it); });
    for_each(_imageAvailableSemaphores, [this](const auto& it) { _device.destroySemaphore(it); });
}

// =====================================================================================================================
// RENDERING & DRAWING
// =====================================================================================================================

void tpd::renderer::RasterRenderer::render() {
    render([](const uint32_t) {});
}

void tpd::renderer::RasterRenderer::render(const std::function<void(uint32_t)>& onFrameReady) {
    uint32_t imageIndex;
    if (!beginFrame(&imageIndex)) return;
    onFrameReady(_currentFrame);

    beginRenderPass(imageIndex);
    onDraw(_drawingCommandBuffers[_currentFrame]);
    endFrame(imageIndex);

    _currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

bool tpd::renderer::RasterRenderer::beginFrame(uint32_t* imageIndex) {
    using limits = std::numeric_limits<uint64_t>;
    [[maybe_unused]] const auto result = _device.waitForFences(_drawingInFlightFences[_currentFrame], vk::True, limits::max());

    if (!acquireSwapChainImage(_imageAvailableSemaphores[_currentFrame], imageIndex)) {
        return false;
    }

    _device.resetFences(_drawingInFlightFences[_currentFrame]);
    _drawingCommandBuffers[_currentFrame].reset();
    _drawingCommandBuffers[_currentFrame].begin(vk::CommandBufferBeginInfo{});

    return true;
}

void tpd::renderer::RasterRenderer::endFrame(const uint32_t imageIndex) {
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

void tpd::renderer::RasterRenderer::beginRenderPass(const uint32_t imageIndex) {
    auto renderPassBeginInfo = vk::RenderPassBeginInfo{};
    renderPassBeginInfo.renderPass = _renderPass;
    renderPassBeginInfo.framebuffer = _framebuffers[imageIndex];
    renderPassBeginInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
    renderPassBeginInfo.renderArea.extent = _swapChainImageExtent;
    constexpr auto clearColor = std::array<vk::ClearValue, 1>{ vk::ClearColorValue{ 0.0f, 0.0f, 0.0f, 1.0f } };
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = clearColor.data();

    _drawingCommandBuffers[_currentFrame].beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
}

void tpd::renderer::RasterRenderer::onDestroy(const vk::Instance instance) noexcept {
    destroyDrawingSyncObjects();
    _device.destroyRenderPass(_renderPass);
    cleanupSwapChain();
    instance.destroySurfaceKHR(_surface);
    Renderer::onDestroy(instance);
}