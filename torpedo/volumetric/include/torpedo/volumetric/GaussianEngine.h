#pragma once

#include "torpedo/rendering/Engine.h"

#include <torpedo/foundation/Target.h>
#include <torpedo/foundation/ReadbackBuffer.h>
#include <torpedo/foundation/RingBuffer.h>
#include <torpedo/foundation/ShaderLayout.h>
#include <torpedo/foundation/StorageBuffer.h>

#include <vsg/maths/mat4.h>
#include <vsg/maths/quat.h>
#include <vsg/maths/vec2.h>
#include <vsg/maths/vec4.h>

namespace tpd {
    class GaussianEngine final : public Engine {
    public:
        void preFrameCompute() const;
        void draw(SwapImage image) override;

        ~GaussianEngine() noexcept override { destroy(); }

    private:
        [[nodiscard]] PhysicalDeviceSelection pickPhysicalDevice(
            const std::vector<const char*>& deviceExtensions,
            vk::Instance instance, vk::SurfaceKHR surface) const override;

        [[nodiscard]] vk::Device createDevice(
            const std::vector<const char*>& deviceExtensions,
            std::initializer_list<uint32_t> queueFamilyIndices) const override;
        
        static vk::PhysicalDeviceVulkan13Features getVulkan13Features();

        [[nodiscard]] const char* getName() const noexcept override;
        [[nodiscard]] std::pmr::memory_resource* getFrameResource() noexcept override;
        [[nodiscard]] bool asyncCompute() const noexcept;

        void onInitialized() override;
        void logDebugInfos() const noexcept;

        static void framebufferResizeCallback(void* ptr, uint32_t width, uint32_t height);
        void onFramebufferResize(uint32_t width, uint32_t height);

        void createDrawingCommandPool();
        void createComputeCommandPool(); // only called if async compute is used

        void createFrames();

        void createRenderTargets(uint32_t width, uint32_t height);
        void cleanupRenderTargets() noexcept;

        void createPointCloudObject(uint32_t width, uint32_t height);

        void createCameraObject(uint32_t width, uint32_t height);
        void createCameraBuffer();
        void updateCameraBuffer(uint32_t currentFrame) const;

        void createGaussianBuffer();
        void createSplatBuffer();
        void createPrefixOffsetsBuffer();
        void createTilesRenderedBuffer();

        void createPreparePipeline();
        void createPrefixPipeline();
        void createForwardPipeline();

        [[nodiscard]] vk::Pipeline createPipeline(const std::string& slangFile, vk::PipelineLayout layout) const;
        void setTargetDescriptors() const;
        void setStorageBufferDescriptors(
            vk::Buffer buffer, std::size_t size, const ShaderInstance& instance,
            uint32_t binding, uint32_t set = 0) const;

        static constexpr uint32_t LINEAR_WORKGROUP_SIZE = 256; // number of local threads per workgroup in scan passes
        static constexpr uint32_t BLOCK_X = 16; // tile size in x-dimension
        static constexpr uint32_t BLOCK_Y = 16; // tile size in y-dimension
        void recordPreprocess(vk::CommandBuffer cmd, uint32_t frameIndex) const noexcept;
        void recordFinalBlend(vk::CommandBuffer cmd, uint32_t frameIndex) const noexcept;
        void recordTargetCopy(vk::CommandBuffer cmd, SwapImage swapImage, uint32_t frameIndex) const noexcept;

        void destroy() noexcept override;

        struct Frame {
            vk::CommandBuffer drawing{};
            vk::CommandBuffer compute{};
            vk::Semaphore ownership{}; // only initialize if async compute is used
            vk::Fence preFrameFence{};
            vk::Fence readBackFence{};
            Target renderTarget{};
        };

        struct PointCloud {
            uint32_t count;
            uint32_t shDegree;
            vsg::uivec2 imgSize;
        };

        static constexpr auto NEAR = 0.2f;
        static constexpr auto FAR = 10.0f;
        struct Camera {
            vsg::mat4 viewMatrix;
            vsg::mat4 projMatrix;
            vsg::vec2 tanFov;
        };

        vk::Queue _graphicsQueue;
        vk::Queue _computeQueue;

        std::pmr::unsynchronized_pool_resource _frameResource{};
        std::pmr::vector<Frame> _frames{ &_frameResource };
        PointCloud _pc{};
        Camera _camera{};
        RingBuffer _cameraBuffer{};

        vk::CommandPool _drawingCommandPool{};
        vk::CommandPool _computeCommandPool{};
        std::vector<vk::ImageView> _targetViews{};
        std::unique_ptr<TransferWorker> _transferWorker{};

        static constexpr uint32_t GAUSSIAN_COUNT = 1;
        static constexpr uint32_t SPLAT_SIZE = 48; // check splat.slang

        // The maximum number of floats for RGB spherical harmonics
        static constexpr uint32_t MAX_SH_RGB = 48;
        struct Gaussian {
            vsg::vec3 position;
            float opacity;
            vsg::quat quaternion;
            vsg::vec4 scale;
            std::array<float, MAX_SH_RGB> sh;
        };

        StorageBuffer _gaussianBuffer{};
        StorageBuffer _splatBuffer{};
        StorageBuffer _prefixOffsetsBuffer{};
        ReadbackBuffer _tilesRenderedBuffer{};

        ShaderLayout _prepareLayout{};
        ShaderInstance _prepareInstance{};
        vk::Pipeline _preparePipeline{};
        vk::PipelineLayout _preparePipelineLayout{};

        ShaderLayout _prefixLayout{};
        ShaderInstance _prefixInstance{};
        vk::Pipeline _prefixPipeline{};
        vk::PipelineLayout _prefixPipelineLayout{};

        ShaderLayout _forwardLayout{};
        ShaderInstance _forwardInstance{};
        vk::Pipeline _forwardPipeline{};
        vk::PipelineLayout _forwardPipelineLayout{};
    };
} // namespace tpd

inline const char* tpd::GaussianEngine::getName() const noexcept {
    return "tpd::GaussianEngine";
}

inline std::pmr::memory_resource* tpd::GaussianEngine::getFrameResource() noexcept {
    return &_frameResource;
}

inline bool tpd::GaussianEngine::asyncCompute() const noexcept {
    return _graphicsFamilyIndex != _computeFamilyIndex;
}
