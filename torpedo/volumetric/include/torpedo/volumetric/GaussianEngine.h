#pragma once

#include <torpedo/rendering/Engine.h>
#include <torpedo/rendering/Camera.h>

#include <torpedo/foundation/ReadbackBuffer.h>
#include <torpedo/foundation/RingBuffer.h>
#include <torpedo/foundation/ShaderLayout.h>
#include <torpedo/foundation/StorageBuffer.h>
#include <torpedo/foundation/Target.h>
#include <torpedo/foundation/TransferWorker.h>

#include <torpedo/math/mat4.h>

namespace tpd {
    class GaussianEngine final : public Engine {
    public:
        void preFrameCompute(const Camera& camera);
        void draw(SwapImage image) const;

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
        void createComputeCommandPool(); // only called if async compute is being used

        void createGaussianLayout();
        [[nodiscard]] vk::Pipeline createPipeline(const std::string& slangFile, vk::PipelineLayout layout) const;

        void createFrames();

        void createRenderTargets(uint32_t width, uint32_t height);
        void cleanupRenderTargets() noexcept;

        void createPointCloudObject();

        void createCameraBuffer();
        void createGaussianBuffer();
        void createSplatBuffer();
        void createTilesRenderedBuffer();
        void createPartitionCountBuffer();
        void createPartitionDescriptorBuffer();
        void createSplatKeyBuffer(uint32_t frameIndex);
        void createSplatIndexBuffer(uint32_t frameIndex);
        void createBlockDescriptorBuffers(uint32_t frameIndex);
        void createBlockCountBuffers();
        void createGlobalSumBuffers();
        void createGlobalPrefixBuffer(uint32_t frameIndex);
        void createTempKeyBuffers(uint32_t frameIndex);
        void createTempValBuffers(uint32_t frameIndex);
        void createRangeBuffers(uint32_t width, uint32_t height);

        void setStorageBufferDescriptors(vk::Buffer buffer, vk::DeviceSize size, uint32_t binding) const;

        void updateCameraBuffer(const Camera& camera) const;
        void recordSplat(vk::CommandBuffer cmd) const noexcept;
        void reallocateBuffers(uint32_t frameIndex);
        void recordBlend(vk::CommandBuffer cmd, uint32_t tilesRendered, uint32_t frameIndex) const noexcept;
        void recordTargetCopy(vk::CommandBuffer cmd, SwapImage swapImage, uint32_t frameIndex) const noexcept;

        void destroy() noexcept override;

        struct Frame {
            ShaderInstance<1> instance{};
            vk::CommandBuffer drawing{};
            vk::CommandBuffer compute{};
            vk::Semaphore ownership{}; // only initialize if async compute is being used
            vk::Fence preFrameFence{};
            vk::Fence readBackFence{};
            uint32_t maxTilesRendered{};
            StorageBuffer rangeBuffer{}; // put this here so that we can quickly reference it for buffer clearing
            Target outputImage{};
        };

        // This is the immutable part of the RasterInfo struct in splat.slang during frame drawing. This separation is
        // due to the fact that the tilesRendered member could change half-way through the pre-frame compute pass.
        struct PointCloud {
            uint32_t count;
            uint32_t shDegree;
        };

        static constexpr uint32_t WORKGROUP_SIZE = 256; // number of local threads per workgroup in scan passes
        static constexpr uint32_t BLOCK_X = 16; // tile size in x-dimension
        static constexpr uint32_t BLOCK_Y = 16; // tile size in y-dimension
        static constexpr uint32_t SPLAT_SIZE = 48; // check splat.slang
        static constexpr uint32_t MAX_SH_RGB = 48; // maximum number of floats for RGB spherical harmonics

        /*--------------------*/

        std::pmr::unsynchronized_pool_resource _frameResource{};
        vk::CommandPool _drawingCommandPool{};
        vk::CommandPool _computeCommandPool{};
        std::vector<vk::ImageView> _targetViews{};

        /*--------------------*/

        vk::Queue _graphicsQueue;
        vk::Queue _computeQueue;
        std::pmr::vector<Frame> _frames{ &_frameResource };
        vk::PipelineLayout _gaussianLayout{};

        /*--------------------*/

        PointCloud _pc{};
        RingBuffer _cameraBuffer{};
        ReadbackBuffer _tilesRenderedBuffer{};

        /*--------------------*/

        vk::Pipeline _projectPipeline{};
        vk::Pipeline _prefixPipeline{};
        vk::Pipeline _keygenPipeline{};
        vk::Pipeline _radixPipeline{};
        vk::Pipeline _coalescePipeline{};
        vk::Pipeline _rangePipeline{};
        vk::Pipeline _forwardPipeline{};

        /*--------------------*/

        static constexpr uint32_t GAUSSIAN_COUNT = 8192;
        struct Gaussian {
            vec3 position;
            float opacity;
            vec4 quaternion;
            vec4 scale;
            std::array<float, MAX_SH_RGB> sh;
        };

        ShaderLayout<1> _shaderLayout{};
        std::unique_ptr<TransferWorker> _transferWorker{};

        StorageBuffer _gaussianBuffer{};
        StorageBuffer _splatBuffer{};
        StorageBuffer _partitionCountBuffer{};
        StorageBuffer _partitionDescriptorBuffer{};
        std::vector<StorageBuffer> _splatKeyBuffers{};
        std::vector<StorageBuffer> _splatIndexBuffers{};
        std::vector<StorageBuffer> _blockCountBuffers{};
        std::vector<StorageBuffer> _blockDescriptorBuffer0s{};
        std::vector<StorageBuffer> _blockDescriptorBuffer1s{};
        std::vector<StorageBuffer> _globalSumBuffers{};
        std::vector<StorageBuffer> _globalPrefixBuffers{};
        std::vector<StorageBuffer> _tempKeyBuffers{};
        std::vector<StorageBuffer> _tempValBuffers{};

        using PipelineStage = vk::PipelineStageFlagBits2;
        using AccessMask = vk::AccessFlagBits2;

        static constexpr auto RAW_BARRIER = vk::MemoryBarrier2{
            PipelineStage::eComputeShader,   // src stage
            AccessMask::eShaderStorageWrite, // src access
            PipelineStage::eComputeShader,   // dst stage
            AccessMask::eShaderStorageRead,  // dst access
        };

        static constexpr auto WAW_BARRIER = vk::MemoryBarrier2{
            PipelineStage::eComputeShader,
            AccessMask::eShaderStorageWrite,
            PipelineStage::eComputeShader,
            AccessMask::eShaderStorageRead | AccessMask::eShaderStorageWrite,
        };

        static constexpr auto RAW_DEPENDENCY = vk::DependencyInfo{ {}, 1, &RAW_BARRIER };
        static constexpr auto WAW_DEPENDENCY = vk::DependencyInfo{ {}, 1, &WAW_BARRIER };
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
