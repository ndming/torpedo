#pragma once

#include <torpedo/rendering/Engine.h>

#include <torpedo/foundation/Target.h>
#include <torpedo/foundation/ReadbackBuffer.h>
#include <torpedo/foundation/RingBuffer.h>
#include <torpedo/foundation/ShaderLayout.h>
#include <torpedo/foundation/StorageBuffer.h>

#include <torpedo/math/mat4.h>

namespace tpd {
    class GaussianEngine final : public Engine {
    public:
        void preFrameCompute();
        void draw(SwapImage image) override;

        ~GaussianEngine() noexcept override { destroy(); }

    private:
        [[nodiscard]] PhysicalDeviceSelection pickPhysicalDevice(
            const std::vector<const char*>& deviceExtensions,
            vk::Instance instance, vk::SurfaceKHR surface) const override;

        [[nodiscard]] vk::Device createDevice(
            const std::vector<const char*>& deviceExtensions,
            std::initializer_list<uint32_t> queueFamilyIndices) const override;

        static vk::PhysicalDeviceFeatures getFeatures();
        static vk::PhysicalDeviceVulkan13Features getVulkan13Features();
        static vk::PhysicalDeviceShaderAtomicInt64Features getShaderAtomicInt64Features();

        [[nodiscard]] const char* getName() const noexcept override;
        [[nodiscard]] std::pmr::memory_resource* getFrameResource() noexcept override;
        [[nodiscard]] bool asyncCompute() const noexcept;

        void onInitialized() override;
        void logDebugInfos() const noexcept;

        static void framebufferResizeCallback(void* ptr, uint32_t width, uint32_t height);
        void onFramebufferResize(uint32_t width, uint32_t height);

        void createDrawingCommandPool();
        void createComputeCommandPool(); // only called if async compute is used

        void createGaussianLayout();
        [[nodiscard]] vk::Pipeline createPipeline(const std::string& slangFile, vk::PipelineLayout layout) const;

        void createFrames();

        void createRenderTargets(uint32_t width, uint32_t height);
        void cleanupRenderTargets() noexcept;

        void createPointCloudObject();

        void createCameraObject(uint32_t width, uint32_t height);
        void updateCameraBuffer(uint32_t frameIndex) const;

        void createCameraBuffer();
        void createGaussianBuffer();
        void createSplatBuffer();
        void createTilesRenderedBuffer();
        void createPartitionCountBuffer();
        void createPartitionDescriptorBuffer();
        void createKeyBuffer();
        void createValBuffer();
        void createBlockSumBuffer();
        void createLocalSumBuffer();
        void createSortedKeyBuffer();
        void createSplatIndexBuffer();
        void createRangeBuffer(uint32_t width, uint32_t height);

        void setStorageBufferDescriptors(
            vk::Buffer buffer, vk::DeviceSize size, const ShaderInstance& instance,
            uint32_t binding, uint32_t set = 0) const;

        static constexpr uint32_t WORKGROUP_SIZE = 256; // number of local threads per workgroup in scan passes
        static constexpr uint32_t BLOCK_X = 16; // tile size in x-dimension
        static constexpr uint32_t BLOCK_Y = 16; // tile size in y-dimension
        void recordSplat(vk::CommandBuffer cmd) const noexcept;
        void reallocateBuffers();
        void recordBlend(vk::CommandBuffer cmd) const noexcept;
        void recordTargetCopy(vk::CommandBuffer cmd, SwapImage swapImage, uint32_t frameIndex) const noexcept;

        void destroy() noexcept override;

        struct Frame {
            vk::CommandBuffer drawing{};
            vk::CommandBuffer compute{};
            vk::Semaphore ownership{}; // only initialize if async compute is being used
            vk::Fence preFrameFence{};
            vk::Fence readBackFence{};
            Target outputImage{};
        };

        // This is the immutable part of the RasterInfo struct in splat.slang during frame drawing. This separation is
        // due to the fact that the tilesRendered member could change half-way through the pre-frame compute pass.
        struct PointCloud {
            uint32_t count;
            uint32_t shDegree;
        };

        static constexpr auto NEAR = 0.2f;
        static constexpr auto FAR = 10.0f;
        struct Camera {
            mat4 viewMatrix;
            mat4 projMatrix;
            vec2 tanFov;
        };

        /*--------------------*/

        std::pmr::unsynchronized_pool_resource _frameResource{};
        vk::CommandPool _drawingCommandPool{};
        vk::CommandPool _computeCommandPool{};
        std::vector<vk::ImageView> _targetViews{};

        /*--------------------*/

        vk::Queue _graphicsQueue;
        vk::Queue _computeQueue;
        std::pmr::vector<Frame> _frames{ &_frameResource };
        uint32_t _currentTilesRendered{ 1 };

        /*--------------------*/

        ShaderInstance _shaderInstance{};

        /*--------------------*/

        vk::PipelineLayout _gaussianLayout{};
        vk::Pipeline _projectPipeline{};
        vk::Pipeline _prefixPipeline{};
        vk::Pipeline _keygenPipeline{};
        vk::Pipeline _radixPipeline{};
        vk::Pipeline _coalescePipeline{};
        vk::Pipeline _rangePipeline{};
        vk::Pipeline _forwardPipeline{};

        /*--------------------*/

        PointCloud _pc{};
        RingBuffer _cameraBuffer{};
        ReadbackBuffer _tilesRenderedBuffer{};

        /*--------------------*/

        Camera _camera{};

        /*--------------------*/

        static constexpr uint32_t GAUSSIAN_COUNT = 16;
        static constexpr uint32_t SPLAT_SIZE = 48; // check splat.slang

        // The maximum number of floats for RGB spherical harmonics
        static constexpr uint32_t MAX_SH_RGB = 48;
        struct Gaussian {
            vec3 position;
            float opacity;
            vec4 quaternion;
            vec4 scale;
            std::array<float, MAX_SH_RGB> sh;
        };

        ShaderLayout _shaderLayout{};
        std::unique_ptr<TransferWorker> _transferWorker{};

        StorageBuffer _gaussianBuffer{};
        StorageBuffer _splatBuffer{};
        StorageBuffer _partitionCountBuffer{};
        StorageBuffer _partitionDescriptorBuffer{};
        StorageBuffer _keyBuffer{};
        StorageBuffer _valBuffer{};
        StorageBuffer _blockSumBuffer{};
        StorageBuffer _localSumBuffer{};
        StorageBuffer _sortedKeyBuffer{};
        StorageBuffer _splatIndexBuffer{};
        StorageBuffer _rangeBuffer{};
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
