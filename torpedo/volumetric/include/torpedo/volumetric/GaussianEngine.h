#pragma once

#include <torpedo/rendering/Engine.h>
#include <torpedo/foundation/Target.h>
#include <torpedo/foundation/ReadbackBuffer.h>
#include <torpedo/foundation/RingBuffer.h>
#include <torpedo/foundation/ShaderLayout.h>

#include <vsg/maths/mat4.h>
#include <vsg/maths/quat.h>
#include <vsg/maths/vec2.h>
#include <vsg/maths/vec4.h>

namespace tpd {
    class GaussianEngine final : public Engine {
    public:
        void preFrameCompute();

        [[nodiscard]] DrawPackage draw(vk::Image image) override;

        [[nodiscard]] const char* getName() const noexcept override;

        ~GaussianEngine() noexcept override;

    private:
        [[nodiscard]] PhysicalDeviceSelection pickPhysicalDevice(
            const std::vector<const char*>& deviceExtensions,
            vk::Instance instance, vk::SurfaceKHR surface) const override;

        [[nodiscard]] vk::Device createDevice(
            const std::vector<const char*>& deviceExtensions,
            std::initializer_list<uint32_t> queueFamilyIndices) const override;
        
        static vk::PhysicalDeviceVulkan13Features getVulkan13Features();

        void onInitialized() override;

        static void framebufferResizeCallback(void* ptr, uint32_t width, uint32_t height);
        void onFramebufferResize(uint32_t width, uint32_t height);

        struct PointCloud {
            uint32_t count;
            uint32_t shDegree;
            vsg::uivec2 imgSize;
        };
        void createPointCloudObject(uint32_t width, uint32_t height);
        PointCloud _pc{};

        static constexpr auto NEAR = 0.2f;
        static constexpr auto FAR = 10.0f;

        struct Camera {
            vsg::mat4 viewMatrix;
            vsg::mat4 projMatrix;
            vsg::vec2 tanFov;
        };
        void createCameraObject(uint32_t width, uint32_t height);
        Camera _camera{};

        static constexpr uint32_t LOCAL_SCAN = 256; // number of local threads per workgroup in scan passes
        static constexpr uint32_t MAX_SH_RGB = 48;  // the maximum number of floats for RGB spherical harmonics
        static constexpr uint32_t BLOCK_X = 16; // tile size in x-dimension
        static constexpr uint32_t BLOCK_Y = 16; // tile size in y-dimension

        void createCameraBuffer();
        void updateCameraBuffer(uint32_t currentFrame) const;
        std::unique_ptr<RingBuffer, Deleter<RingBuffer>> _cameraBuffer{};

        struct Gaussian {
            vsg::vec3 position;
            float opacity;
            vsg::quat quaternion;
            vsg::vec4 scale;
            std::array<float, MAX_SH_RGB> sh;
        };

        void createGaussianBuffer();
        static constexpr uint32_t GAUSSIAN_COUNT = 1;
        std::unique_ptr<StorageBuffer, Deleter<StorageBuffer>> _gaussianBuffer{};

        void createSplatBuffer();
        static constexpr uint32_t SPLAT_SIZE = 48; // check splat.slang
        std::unique_ptr<StorageBuffer, Deleter<StorageBuffer>> _splatBuffer{};

        void createPrefixOffsetsBuffer();
        std::unique_ptr<StorageBuffer, Deleter<StorageBuffer>> _prefixOffsetsBuffer{};

        void createTilesRenderedBuffer();
        std::unique_ptr<ReadbackBuffer, Deleter<ReadbackBuffer>> _tilesRenderedBuffer{};

        void createRenderTargets(uint32_t width, uint32_t height);
        std::pmr::vector<Target> _targets{ &_engineResourcePool };
        std::pmr::vector<vk::ImageView> _targetViews{ &_engineResourcePool };

        void createPreparePipeline();
        std::unique_ptr<ShaderLayout, Deleter<ShaderLayout>> _prepareLayout{};
        std::unique_ptr<ShaderInstance, Deleter<ShaderInstance>> _prepareInstance{};
        vk::Pipeline _preparePipeline{};

        void createPrefixPipeline();
        std::unique_ptr<ShaderLayout, Deleter<ShaderLayout>> _prefixLayout{};
        std::unique_ptr<ShaderInstance, Deleter<ShaderInstance>> _prefixInstance{};
        vk::Pipeline _prefixPipeline{};

        void createForwardPipeline();
        std::unique_ptr<ShaderLayout, Deleter<ShaderLayout>> _forwardLayout{};
        std::unique_ptr<ShaderInstance, Deleter<ShaderInstance>> _forwardInstance{};
        vk::Pipeline _forwardPipeline{};

        [[nodiscard]] vk::Pipeline buildComputePipeline(const std::string& slangFile, vk::PipelineLayout layout) const;
        void setTargetDescriptors() const;
        void setStorageBufferDescriptors(
            vk::Buffer buffer, std::size_t size, const ShaderInstance& instance,
            uint32_t binding, uint32_t set = 0) const;

        void createComputeCommandPool(); // only called if async compute is used
        vk::CommandPool _computeCommandPool{};

        void createOwnershipSemaphores(); // only called if async compute is used
        std::pmr::vector<vk::Semaphore> _ownershipSemaphores{ &_engineResourcePool };

        void createPreFrameCommandBuffers();
        std::pmr::vector<vk::CommandBuffer> _preFrameCommandBuffers{ &_engineResourcePool };

        void createFences();
        std::pmr::vector<vk::Fence> _preFrameFences{ &_engineResourcePool };
        std::pmr::vector<vk::Fence> _readBackFences{ &_engineResourcePool };

        void recordPreprocess(vk::CommandBuffer cmd, uint32_t currentFrame) const;
        void recordFinalBlend(vk::CommandBuffer cmd, uint32_t currentFrame);
        void recordTargetCopy(vk::CommandBuffer cmd, vk::Image swapImage, uint32_t currentFrame) const;

        void cleanupRenderTargets() noexcept;
        void destroy() noexcept override;
    };
} // namespace tpd

inline const char* tpd::GaussianEngine::getName() const noexcept {
    return "tpd::GaussianEngine";
}

inline tpd::GaussianEngine::~GaussianEngine() noexcept {
    destroy();
}
