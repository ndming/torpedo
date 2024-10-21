#pragma once

#include "torpedo/graphics/Renderer.h"

#include <GLFW/glfw3.h>

namespace tpd {
    class StandardRenderer : public Renderer {
    public:
        StandardRenderer(const StandardRenderer&) = delete;
        StandardRenderer& operator=(const StandardRenderer&) = delete;

        void render(const View& view);
        void render(const View& view, const std::function<void(uint32_t)>& onFrameReady) override;

        void setOnFramebufferResize(const std::function<void(uint32_t, uint32_t)> &callback);
        void setOnFramebufferResize(std::function<void(uint32_t, uint32_t)> &&callback) noexcept;
        [[nodiscard]] std::pair<uint32_t, uint32_t> getFramebufferSize() const final;

        ~StandardRenderer() override;

    protected:
        explicit StandardRenderer(GLFWwindow* window);

        GLFWwindow* _window;
        vk::SurfaceKHR _surface{};

        static std::vector<const char*> getDeviceExtensions();

        void registerFeatures() override;
        [[nodiscard]] static vk::PhysicalDeviceFeatures getFeatures();
        [[nodiscard]] static vk::PhysicalDeviceVertexInputDynamicStateFeaturesEXT getVertexInputDynamicStateFeatures();
        [[nodiscard]] static vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT getExtendedDynamicStateFeatures();
        [[nodiscard]] static vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT getExtendedDynamicState3Features();

        static void loadExtensionFunctions(vk::Instance instance);
        static PFN_vkCmdSetVertexInputEXT _vkCmdSetVertexInput;
        static PFN_vkCmdSetPolygonModeEXT _vkCmdSetPolygonMode;
        static PFN_vkCmdSetRasterizationSamplesEXT _vkCmdSetRasterizationSamples;

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

        // Drawing
        virtual void beginRenderPass(uint32_t imageIndex) const;
        [[nodiscard]] virtual std::vector<vk::ClearValue> getClearValues() const;
        virtual void onDraw(const View& view, vk::CommandBuffer buffer) const = 0;

    private:
        static std::vector<const char*> getRequiredExtensions();

        bool _framebufferResized{ false };
        std::function<void(uint32_t, uint32_t)> _userFramebufferResizeCallback{ [](uint32_t, uint32_t) {} };
        static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

        void createSurface();
        void init() final;

        // Present queue family
        uint32_t _presentQueueFamily{};
        vk::Queue _presentQueue{};
        void pickPhysicalDevice() final;
        void createDevice() final;

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

        void updateSharedObjects(const View& view) const;
        void updateCameraObject(const Camera& camera) const;
        void updateLightObject(const Scene& scene) const;

        template<Renderable R>
        friend std::unique_ptr<R> createRenderer(void* nativeWindow);
    };
}

inline PFN_vkCmdSetVertexInputEXT tpd::StandardRenderer::_vkCmdSetVertexInput = nullptr;
inline PFN_vkCmdSetPolygonModeEXT tpd::StandardRenderer::_vkCmdSetPolygonMode = nullptr;
inline PFN_vkCmdSetRasterizationSamplesEXT tpd::StandardRenderer::_vkCmdSetRasterizationSamples = nullptr;

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline void tpd::StandardRenderer::setOnFramebufferResize(const std::function<void(uint32_t, uint32_t)>& callback) {
    _userFramebufferResizeCallback = callback;
}

inline void tpd::StandardRenderer::setOnFramebufferResize(std::function<void(uint32_t, uint32_t)>&& callback) noexcept {
    _userFramebufferResizeCallback = std::move(callback);
}

inline std::pair<uint32_t, uint32_t> tpd::StandardRenderer::getFramebufferSize() const {
    return { _swapChainImageExtent.width, _swapChainImageExtent.height };
}
