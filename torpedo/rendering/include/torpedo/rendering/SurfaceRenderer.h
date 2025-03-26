#pragma once

#include "torpedo/rendering/Renderer.h"

#include <torpedo/foundation/AllocationUtils.h>

#include <GLFW/glfw3.h>
#include <functional>

namespace tpd {
    class SurfaceRenderer final : public Renderer {
    public:
        class Window final {
        public:
            explicit Window(vk::Extent2D initialFramebufferSize);
            explicit Window(bool fullscreen);

            Window(const Window&) = delete;
            Window& operator=(const Window&) = delete;

            void setTitle(std::string_view title) const;

            void loop(const std::function<void()>&      onRender) const;
            void loop(const std::function<void(float)>& onRender) const;

            ~Window();

        private:
            GLFWwindow* _glfwWindow;
            friend class SurfaceRenderer;
        };

        struct Presentable {
            bool valid{ false };
            vk::Image image{};
            uint32_t imageIndex{};
        };

        [[nodiscard]] Presentable launchFrame();
        void submitFrame(
            vk::CommandBuffer buffer, 
            vk::PipelineStageFlags2 waitStage, 
            vk::PipelineStageFlags2 doneStage, 
            uint32_t imageIndex);

        [[nodiscard]] const std::unique_ptr<Window, Deleter<Window>>& getWindow() const;

        [[nodiscard]] vk::Extent2D getFramebufferSize() const noexcept override;
        [[nodiscard]] uint32_t getInFlightFramesCount() const noexcept override;
        [[nodiscard]] uint32_t getCurrentDrawingFrame() const noexcept override;
        [[nodiscard]] bool hasSurfaceRenderingSupport() const noexcept override;

        void resetEngine() noexcept override;
        ~SurfaceRenderer() noexcept override;

    private:
        [[nodiscard]] std::vector<const char*> getInstanceExtensions() const override;
        [[nodiscard]] std::vector<const char*> getDeviceExtensions() const override;

        SurfaceRenderer() = default;
        void init(uint32_t frameWidth, uint32_t frameHeight, std::pmr::memory_resource* contextPool) override;
        void init(bool fullscreen, std::pmr::memory_resource* contextPool);
        std::unique_ptr<Window, Deleter<Window>> _window{};

        static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
        bool _framebufferResized{ false };

        [[nodiscard]] vk::SurfaceKHR getVulkanSurface() const override;
        void createSurface();
        vk::SurfaceKHR _surface{};

        void engineInit(uint32_t graphicsFamilyIndex, uint32_t presentFamilyIndex) override;
        uint32_t _graphicsFamilyIndex{ 0 };
        uint32_t _presentFamilyIndex { 0 };
        vk::Queue _graphicsQueue{};
        vk::Queue _presentQueue {};

        void createSwapChain();
        vk::SwapchainKHR _swapChain{};

        void createSwapChainImageViews();
        std::vector<vk::Image> _swapChainImages{};
        std::vector<vk::ImageView> _swapChainImageViews{};

        vk::Format _swapChainImageFormat{ vk::Format::eUndefined };
        vk::Extent2D _swapChainImageExtent{};

        struct FrameSync {
            vk::Semaphore imageReady{};
            vk::Semaphore renderDone{};
            vk::Fence frameDrawFence{};
        };

        static constexpr uint32_t IN_FLIGHT_FRAMES{ 2 };

        void createSyncPrimitives();
        std::array<FrameSync, IN_FLIGHT_FRAMES> _syncs{};

        bool acquireSwapChainImage(vk::Semaphore semaphore, uint32_t* imageIndex);
        void presentSwapChainImage(uint32_t imageIndex, vk::Semaphore semaphore);
        uint32_t _currentFrame{ 0 };

        void refreshSwapChain();
        void cleanupSwapChain() const noexcept;

        void destroy() noexcept override;

        template<RendererImpl R>
        friend class Context;
    };
}

inline void tpd::SurfaceRenderer::Window::setTitle(const std::string_view title) const {
    glfwSetWindowTitle(_glfwWindow, title.data());
}

inline const std::unique_ptr<tpd::SurfaceRenderer::Window, tpd::Deleter<tpd::SurfaceRenderer::Window>>& tpd::SurfaceRenderer::getWindow() const {
    if (!_initialized) [[unlikely]] {
        throw std::runtime_error(
            "SurfaceRenderer - getWindow called before initialization: "
            "did you forget to call SurfaceRenderer::init()?");
    }
    return _window;
}

inline uint32_t tpd::SurfaceRenderer::getInFlightFramesCount() const noexcept {
    return IN_FLIGHT_FRAMES;
}

inline uint32_t tpd::SurfaceRenderer::getCurrentDrawingFrame() const noexcept {
    return _currentFrame;
}

inline bool tpd::SurfaceRenderer::hasSurfaceRenderingSupport() const noexcept {
    return true;
}

inline vk::SurfaceKHR tpd::SurfaceRenderer::getVulkanSurface() const {
    return _surface;
}
