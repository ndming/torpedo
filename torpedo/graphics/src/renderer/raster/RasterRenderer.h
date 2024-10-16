#pragma once

#include "torpedo/graphics/Renderer.h"

#include <GLFW/glfw3.h>

namespace tpd::renderer {
    class RasterRenderer : public Renderer {
    public:
        RasterRenderer(const RasterRenderer&) = delete;
        RasterRenderer& operator=(const RasterRenderer&) = delete;

    protected:
        explicit RasterRenderer(GLFWwindow* window);

        // Native window and Vulkan surface
        GLFWwindow* _window;
        vk::SurfaceKHR _surface{};

        [[nodiscard]] PhysicalDeviceSelector getPhysicalDeviceSelector(std::initializer_list<const char*> deviceExtensions) const override;
        [[nodiscard]] std::vector<const char*> getRendererExtensions() const override;

        void onFeaturesRegister() override;
        [[nodiscard]] static vk::PhysicalDeviceFeatures getFeatures();
        [[nodiscard]] static vk::PhysicalDeviceVertexInputDynamicStateFeaturesEXT getVertexInputDynamicStateFeatures();
        [[nodiscard]] static vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT getExtendedDynamicStateFeatures();
        [[nodiscard]] static vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT getExtendedDynamicState3Features();

        // Swap chain characteristics
        vk::Format _swapChainImageFormat{ vk::Format::eUndefined };
        vk::Extent2D _swapChainImageExtent{};
        [[nodiscard]] virtual vk::SurfaceFormatKHR chooseSwapSurfaceFormat() const;
        [[nodiscard]] virtual vk::PresentModeKHR chooseSwapPresentMode() const;
        [[nodiscard]] virtual vk::Extent2D chooseSwapExtent() const;

        // Swap chain image views
        std::vector<vk::ImageView> _swapChainImageViews{};
        void createSwapChainImageViews();

        // Frame buffer resources
        virtual void createFramebufferResources() = 0;
        virtual void destroyFramebufferResources() const noexcept = 0;

        // Render pass
        vk::RenderPass _renderPass{};
        virtual void createRenderPass();

        // Framebuffers
        std::vector<vk::Framebuffer> _framebuffers{};
        virtual void createFramebuffers();

        // Rendering
        uint32_t _currentFrame{ 0 };
        void render() override;
        void render(const std::function<void(uint32_t)>& onFrameReady) override;

        // Drawing
        virtual void beginRenderPass(uint32_t imageIndex);
        virtual void onDraw(vk::CommandBuffer buffer) = 0;

        void onDestroy(vk::Instance instance) noexcept override;

    private:
        bool _framebufferResized{ false };
        static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

        void createSurface(vk::Instance instance);
        void onCreate(vk::Instance instance, std::initializer_list<const char*> deviceExtensions) final;
        void onInitialize() final;

        // Present queue family
        uint32_t _presentQueueFamily{};
        vk::Queue _presentQueue{};
        void pickPhysicalDevice(vk::Instance instance, std::initializer_list<const char*> deviceExtensions) final;
        void createDevice(std::initializer_list<const char*> deviceExtensions) final;

        // Swap chain infrastructure
        vk::SwapchainKHR _swapChain{};
        uint32_t _minImageCount{};
        std::vector<vk::Image> _swapChainImages{};
        void createSwapChain();

        // Swap chain utilities
        void recreateSwapChain();
        void cleanupSwapChain() const noexcept;
        bool acquireSwapChainImage(vk::Semaphore semaphore, uint32_t* imageIndex);
        void presentSwapChainImage(uint32_t imageIndex, vk::Semaphore semaphore);

        // Number of in-flight frames
        static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

        // Drawing command buffers
        std::array<vk::CommandBuffer, MAX_FRAMES_IN_FLIGHT> _drawingCommandBuffers{};
        void createDrawingCommandBuffers();

        // Drawing sync objects
        std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> _imageAvailableSemaphores{};
        std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> _renderFinishedSemaphores{};
        std::array<vk::Fence, MAX_FRAMES_IN_FLIGHT> _drawingInFlightFences{};
        void createDrawingSyncObjects();
        void destroyDrawingSyncObjects() const noexcept;

        // Frame drawing
        bool beginFrame(uint32_t* imageIndex);
        void endFrame(uint32_t imageIndex);
    };
}

