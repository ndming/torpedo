#pragma once

#include <torpedo/bootstrap/PhysicalDeviceSelector.h>
#include <torpedo/foundation/PipelineShader.h>
#include <torpedo/foundation/ResourceAllocator.h>

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

class TordemoApplication {
public:
    void run();

    explicit TordemoApplication(const uint32_t maxFramesInFlight = 2) : _maxFramesInFlight(maxFramesInFlight) {
    }

    virtual ~TordemoApplication();

    TordemoApplication(const TordemoApplication&) = delete;
    TordemoApplication& operator=(const TordemoApplication&) = delete;

private:
    // Application stages
    void initWindow();
    void initVulkan();
    void loop();

protected:
    // Bite-sized overrides
    virtual void createFramebufferResources();
    virtual void destroyFramebufferResources() const noexcept;

    virtual void createCommandPools();
    virtual void destroyCommandPools() const noexcept;

    virtual void createSyncObjects();
    virtual void destroySyncObjects() const noexcept;

    virtual void createPipelineResources();
    virtual void destroyPipelineResources() noexcept;

    virtual void drawFrame();

    // Window
    GLFWwindow* _window{ nullptr };
    bool _framebufferResized{ false };
    [[nodiscard]] virtual std::string getWindowTitle() const;
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    // Vulkan instance
    vk::Instance _instance{};
    virtual void createInstance();
    [[nodiscard]] virtual std::vector<const char*> getRequiredExtensions() const;
    [[nodiscard]] virtual vk::ApplicationInfo getApplicationInfo() const;

#ifndef NDEBUG
    // Debug messenger
    vk::DebugUtilsMessengerEXT _debugMessenger{};
    virtual void createDebugMessenger();
#endif

    // Vulkan surface
    vk::SurfaceKHR _surface{};
    virtual void createSurface();

    // Physical device and queue family indices
    vk::PhysicalDevice _physicalDevice{};
    uint32_t _graphicsQueueFamily{};
    uint32_t _presentQueueFamily{};
    uint32_t _computeQueueFamily{};
    virtual void pickPhysicalDevice();
    [[nodiscard]] virtual tpd::PhysicalDeviceSelector getPhysicalDeviceSelector() const;
    [[nodiscard]] virtual std::vector<const char*> getDeviceExtensions() const;

    // Device and queues
    vk::Device _device{};
    vk::Queue _graphicsQueue{};
    vk::Queue _presentQueue{};
    vk::Queue _computeQueue{};
    virtual void createDevice();
    [[nodiscard]] virtual vk::PhysicalDeviceFeatures2 getDeviceFeatures() const;

    // Resource allocator
    std::unique_ptr<tpd::ResourceAllocator> _resourceAllocator{};
    virtual void initResourceAllocator();

    // Swap chain utilities
    void recreateSwapChain();
    void cleanupSwapChain() const noexcept;
    bool acquireSwapChainImage(vk::Semaphore semaphore, uint32_t* imageIndex);
    void presentSwapChainImage(uint32_t imageIndex, vk::Semaphore semaphore);

    // Swap chain infrastructure
    vk::SwapchainKHR _swapChain{};
    uint32_t _minImageCount{};
    std::vector<vk::Image> _swapChainImages{};
    std::vector<vk::ImageView> _swapChainImageViews{};
    vk::Format _swapChainImageFormat{ vk::Format::eUndefined };
    vk::Extent2D _swapChainImageExtent{};
    virtual void createSwapChain();
    virtual void createSwapChainImageViews();
    [[nodiscard]] virtual vk::SurfaceFormatKHR chooseSwapSurfaceFormat() const;
    [[nodiscard]] virtual vk::PresentModeKHR chooseSwapPresentMode() const;
    [[nodiscard]] virtual vk::Extent2D chooseSwapExtent() const;

    // Framebuffer color resources
    vk::Image _framebufferColorImage{};
    vk::ImageView _framebufferColorImageView{};
    VmaAllocation _framebufferColorImageAllocation{ nullptr };
    virtual void createFramebufferColorResources();
    [[nodiscard]] virtual vk::SampleCountFlagBits getFramebufferColorImageSampleCount() const;
    [[nodiscard]] virtual uint32_t getFramebufferColorImageMipLevels() const;

    // Framebuffer depth resources
    vk::Image _framebufferDepthImage{};
    vk::ImageView _framebufferDepthImageView{};
    VmaAllocation _framebufferDepthImageAllocation{ nullptr };
    vk::Format _framebufferDepthImageFormat{ vk::Format::eUndefined };
    virtual void createFramebufferDepthResources();
    [[nodiscard]] virtual vk::Format findFramebufferDepthImageFormat(
        const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) const;
    [[nodiscard]] virtual vk::SampleCountFlagBits getFramebufferDepthImageSampleCount() const;

    // Render pass
    vk::RenderPass _renderPass{};
    virtual void createRenderPass();

    // Framebuffers
    std::vector<vk::Framebuffer> _framebuffers{};
    virtual void createFramebuffers();
    [[nodiscard]] vk::SampleCountFlagBits getOrFallbackSampleCount(vk::SampleCountFlagBits sampleCount) const;

    // Drawing command pool
    vk::CommandPool _drawingCommandPool{};
    virtual void createDrawingCommandPool();

    // Transfer command pool
    vk::CommandPool _transferCommandPool{};
    void createTransferCommandPool();

    const uint32_t _maxFramesInFlight;

    // Drawing command buffers
    std::vector<vk::CommandBuffer> _drawingCommandBuffers{};
    virtual void createCommandBuffers();

    // Drawing sync objects
    std::vector<vk::Semaphore> _imageAvailableSemaphores{};
    std::vector<vk::Semaphore> _renderFinishedSemaphores{};
    std::vector<vk::Fence> _drawingInFlightFences{};
    void createDrawingSyncObjects();

    // Graphics pipeline
    std::unique_ptr<tpd::PipelineShader> _graphicsPipelineShader{};
    virtual void createGraphicsPipelineShader();
    [[nodiscard]] virtual std::vector<vk::DynamicState> getGraphicsPipelineDynamicStates() const;
    [[nodiscard]] virtual vk::PipelineViewportStateCreateInfo getGraphicsPipelineViewportState() const;
    [[nodiscard]] virtual vk::PipelineInputAssemblyStateCreateInfo getGraphicsPipelineInputAssemblyState() const;
    [[nodiscard]] virtual vk::PipelineVertexInputStateCreateInfo getGraphicsPipelineVertexInputState() const;
    [[nodiscard]] virtual vk::PipelineRasterizationStateCreateInfo getGraphicsPipelineRasterizationState() const;
    [[nodiscard]] virtual vk::PipelineMultisampleStateCreateInfo getGraphicsPipelineMultisampleState() const;
    [[nodiscard]] virtual vk::PipelineDepthStencilStateCreateInfo getGraphicsPipelineDepthStencilState() const;
    [[nodiscard]] virtual vk::PipelineColorBlendAttachmentState getGraphicsPipelineColorBlendAttachmentState() const;
    [[nodiscard]] virtual std::unique_ptr<tpd::PipelineShader> buildPipelineShader(vk::GraphicsPipelineCreateInfo* pipelineInfo) const;

    // Drawing frames
    uint32_t _currentFrame{ 0 };
    virtual bool beginFrame(uint32_t* imageIndex);
    virtual void onFrameReady();
    virtual void endFrame(uint32_t imageIndex);

    // Rendering
    virtual void beginRenderPass(uint32_t imageIndex);
    [[nodiscard]] virtual std::vector<vk::ClearValue> getClearValues() const;
    virtual void render(vk::CommandBuffer buffer);

    // Transfer operations
    [[nodiscard]] vk::CommandBuffer beginSingleTimeTransferCommands() const;
    void endSingleTimeTransferCommands(vk::CommandBuffer commandBuffer) const;
};
