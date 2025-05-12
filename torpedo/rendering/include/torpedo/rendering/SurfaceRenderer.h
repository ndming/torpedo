#pragma once

#include "torpedo/rendering/Renderer.h"
#include "torpedo/rendering/Control.h"

#include <unordered_set>

namespace tpd {
    struct SwapImage;
    class SurfaceRenderer;

    class Window final {
    public:
        struct EventListener {
            SurfaceRenderer* renderer;
            std::unordered_set<Control*> controls;
        };

        Window(vk::Extent2D initialFramebufferSize, SurfaceRenderer* renderer);
        Window(bool fullscreen, SurfaceRenderer* renderer);

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        void setTitle(std::string_view title) const;

        template<ControlImpl C>
        [[nodiscard]] std::unique_ptr<C> createControl();

        template<ControlImpl C>
        void destroyControl(std::unique_ptr<C> control) noexcept;

        ~Window();

    private:
        void checkWindowCreated() const;
        GLFWwindow* _glfwWindow;

        void setupEventListener(SurfaceRenderer* renderer) noexcept;
        EventListener _listener;

        friend class SurfaceRenderer;
    };

    class SurfaceRenderer final : public Renderer {
    public:
        void loop(const std::function<void()>& onRender) const;
        void loop(const std::function<void(float)>& onRender) const;

        [[nodiscard]] SwapImage launchFrame();
        void submitFrame(uint32_t imageIndex);

        [[nodiscard]] const std::unique_ptr<Window>& getWindow() const;

        [[nodiscard]] vk::Extent2D getFramebufferSize() const noexcept override;
        [[nodiscard]] uint32_t getInFlightFrameCount() const noexcept override;
        [[nodiscard]] bool supportSurfaceRendering() const noexcept override;

        [[nodiscard]] uint32_t getCurrentFrameIndex() const noexcept override;
        [[nodiscard]] FrameSync getCurrentFrameSync() const noexcept override;

        void resetEngine() noexcept override;
        ~SurfaceRenderer() noexcept override;

        static constexpr uint32_t MAX_SWAP_IMAGES = 3;
        static constexpr uint32_t IN_FLIGHT_FRAME_COUNT = 2;

    private:
        [[nodiscard]] std::vector<const char*> getInstanceExtensions() const override;
        [[nodiscard]] std::vector<const char*> getDeviceExtensions() const override;

        SurfaceRenderer() = default;
        void init(uint32_t frameWidth, uint32_t frameHeight) override;
        void init(bool fullscreen);
        [[nodiscard]] bool initialized() const noexcept override;

        static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
        [[nodiscard]] vk::SurfaceKHR getVulkanSurface() const override;
        void createSurface();

        void engineInit(uint32_t graphicsFamily, uint32_t presentFamily, std::pmr::memory_resource* frameResource) override;
        void createSwapChain();
        void createFrameSyncPrimitives(std::pmr::memory_resource* frameResource);

        bool acquireSwapChainImage(vk::Semaphore semaphore, uint32_t* imageIndex);
        void presentSwapChainImage(uint32_t imageIndex, vk::Semaphore renderDone);

        void refreshSwapChain();
        void cleanupSwapChain() const noexcept;

        void destroy() noexcept override;

        /*--------------------*/

        std::unique_ptr<Window> _window{};
        bool _framebufferResized{ false };
        vk::SurfaceKHR _surface{};
        uint32_t _graphicsFamilyIndex{};
        uint32_t _presentFamilyIndex{};

        vk::SwapchainKHR _swapChain{};
        vk::Extent2D _swapChainImageExtent{};
        vk::Queue _presentQueue{};
        uint32_t _currentFrame{ 0 };

        /*--------------------*/

        std::array<vk::Image, MAX_SWAP_IMAGES> _swapChainImages{};
        std::pmr::vector<FrameSync> _frameSyncs{};

        /*--------------------*/

        template<RendererImpl R>
        friend class Context;
    };
} // namespace tpd

inline void tpd::Window::setTitle(const std::string_view title) const {
    glfwSetWindowTitle(_glfwWindow, title.data());
}

inline const std::unique_ptr<tpd::Window>& tpd::SurfaceRenderer::getWindow() const {
    if (!initialized()) [[unlikely]] {
        throw std::runtime_error(
            "SurfaceRenderer - getWindow called before initialization: "
            "did you forget to call SurfaceRenderer::init()?");
    }
    return _window;
}

inline bool tpd::SurfaceRenderer::initialized() const noexcept {
    return _window != nullptr;
}

template<tpd::ControlImpl C>
std::unique_ptr<C> tpd::Window::createControl() {
    auto control = std::make_unique<C>(_glfwWindow);
    _listener.controls.insert(control.get());
    return control;
}

template<tpd::ControlImpl C>
void tpd::Window::destroyControl(std::unique_ptr<C> control) noexcept {
    if (!control) [[unlikely]] return;
    _listener.controls.erase(control.get());
    control.reset();
}

inline vk::Extent2D tpd::SurfaceRenderer::getFramebufferSize() const noexcept {
    return _swapChainImageExtent;
}

inline vk::SurfaceKHR tpd::SurfaceRenderer::getVulkanSurface() const {
    return _surface;
}

inline uint32_t tpd::SurfaceRenderer::getInFlightFrameCount() const noexcept {
    return IN_FLIGHT_FRAME_COUNT;
}

inline bool tpd::SurfaceRenderer::supportSurfaceRendering() const noexcept {
    return true;
}

inline uint32_t tpd::SurfaceRenderer::getCurrentFrameIndex() const noexcept {
    return _currentFrame;
}

inline tpd::Renderer::FrameSync tpd::SurfaceRenderer::getCurrentFrameSync() const noexcept {
    return _frameSyncs[_currentFrame];
}
