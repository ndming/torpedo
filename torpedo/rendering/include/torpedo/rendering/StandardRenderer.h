#pragma once

#include "torpedo/rendering/Renderer.h"

#include <GLFW/glfw3.h>

#include <functional>

namespace tpd {
    class StandardRenderer final : public Renderer {
    public:
        class Context final {
        public:
            explicit Context(vk::Extent2D initialFramebufferSize);

            Context(const Context&) = delete;
            Context& operator=(const Context&) = delete;

            void setWindowTitle(std::string_view title) const;

            void loop(const std::function<void()>&      onRender) const;
            void loop(const std::function<void(float)>& onRender) const;

            ~Context();

        private:
            GLFWwindow* _window;
            friend class StandardRenderer;
        };

        struct Presentable {
            bool valid{ false };
            vk::Image image{};
            uint32_t imageIndex{};
        };

        StandardRenderer() = default;
        void init(uint32_t framebufferWidth, uint32_t framebufferHeight) override;

        StandardRenderer(const StandardRenderer&) = delete;
        StandardRenderer& operator=(const StandardRenderer&) = delete;

        [[nodiscard]] Presentable beginFrame();
        void endFrame(const vk::ArrayProxy<vk::CommandBuffer>& buffers, uint32_t imageIndex);

        [[nodiscard]] const std::unique_ptr<Context>& getContext() const;

        [[nodiscard]] vk::Extent2D getFramebufferSize() const override;
        [[nodiscard]] uint32_t getInFlightFramesCount() const override;

        [[nodiscard]] uint32_t getCurrentFrame() const override;
        [[nodiscard]] vk::SurfaceKHR getVulkanSurface() const override;

        void resetEngine() noexcept override;
        void reset() noexcept override;

        ~StandardRenderer() noexcept override;

    private:
        [[nodiscard]] std::vector<const char*> getInstanceExtensions() const override;
        std::unique_ptr<Context> _context{};

        static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
        bool _framebufferResized{ false };

        void createSurface();
        vk::SurfaceKHR _surface{};

        [[nodiscard]] std::vector<const char*> getDeviceExtensions() const override;
        void engineInit(vk::Device device, const PhysicalDeviceSelection& physicalDeviceSelection) override;
        uint32_t _graphicsFamilyIndex{ 0 };
        uint32_t _presentFamilyIndex{ 0 };
        vk::Queue _graphicsQueue{};
        vk::Queue _presentQueue{};

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

        static std::string toString(vk::Extent2D extent);
        static std::string toString(vk::PresentModeKHR presentMode);
    };
}

// =========================== //
// INLINE FUNCTION DEFINITIONS //
// =========================== //

inline void tpd::StandardRenderer::Context::setWindowTitle(const std::string_view title) const {
    glfwSetWindowTitle(_window, title.data());
}

inline const std::unique_ptr<tpd::StandardRenderer::Context>& tpd::StandardRenderer::getContext() const {
    if (!_initialized) [[unlikely]] {
        throw std::runtime_error(
            "StandardRenderer - getContext called before initialization: "
            "did you forget to call StandardRenderer::init()?");
    }
    return _context;
}

inline uint32_t tpd::StandardRenderer::getInFlightFramesCount() const {
    return IN_FLIGHT_FRAMES;
}

inline uint32_t tpd::StandardRenderer::getCurrentFrame() const {
    return _currentFrame;
}

inline vk::SurfaceKHR tpd::StandardRenderer::getVulkanSurface() const {
    return _surface;
}

inline std::string tpd::StandardRenderer::toString(const vk::Extent2D extent) {
    return "(" + std::to_string(extent.width) + ", " + std::to_string(extent.height) + ")";
}

inline std::string tpd::StandardRenderer::toString(const vk::PresentModeKHR presentMode) {
    using enum vk::PresentModeKHR;
    switch (presentMode) {
        case eImmediate:               return "Immediate";
        case eMailbox:                 return "Mailbox";
        case eFifo :                   return "Fifo";
        case eFifoRelaxed:             return "FifoRelaxed";
        case eSharedDemandRefresh:     return "SharedDemandRefresh";
        case eSharedContinuousRefresh: return "SharedContinuousRefresh";
        default: return "Unrecognized present mode at enum: " + std::to_string(static_cast<int>(presentMode));
    }
}
