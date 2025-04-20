#pragma once

#include "torpedo/foundation/Image.h"

namespace tpd {
    class Texture final : public Image {
    public:
        Texture() noexcept = default;
        Texture(vk::Format format, vk::Image image, VmaAllocation allocation);

        [[nodiscard]] vk::ImageAspectFlags getAspectMask() const noexcept override;

        [[nodiscard]] bool isDepth() const noexcept;
        [[nodiscard]] bool isStencil() const noexcept;

        void recordStagingCopy(vk::CommandBuffer cmd, vk::Buffer stagingBuffer, vk::Extent3D extent, uint32_t mipLevel = 0) const noexcept;

        void recordMipGen(
            vk::CommandBuffer cmd, vk::PhysicalDevice physicalDevice, vk::Extent2D extent,
            uint32_t mipCount, vk::ImageLayout dstLayout) const;

        void recordReleaseForMipGen(vk::CommandBuffer cmd, uint32_t transferFamily, uint32_t graphicsFamily) const noexcept;
        void recordAcquireForMipGen(vk::CommandBuffer cmd, uint32_t transferFamily, uint32_t graphicsFamily) const noexcept;

        [[nodiscard]] SyncPoint getLayoutTransitionDstSync(vk::ImageLayout newLayout) const noexcept override;
    };
} // namespace tpd

inline tpd::Texture::Texture(const vk::Format format, const vk::Image image, VmaAllocation allocation)
    : Image{ format, image, allocation } {}

inline tpd::SyncPoint tpd::Texture::getLayoutTransitionDstSync(const vk::ImageLayout newLayout) const noexcept {
    using PipelineStage = vk::PipelineStageFlagBits2;
    using AccessMask    = vk::AccessFlagBits2;

    constexpr auto depthStage   = PipelineStage::eEarlyFragmentTests | PipelineStage::eLateFragmentTests;
    constexpr auto sampledStage = PipelineStage::eVertexShader | PipelineStage::eFragmentShader | PipelineStage::eComputeShader;

    // Override to relax the dst access to shader sampled read only, the default dst access
    // includes attachment read which is not applicable to textures
    using enum vk::ImageLayout;
    switch (newLayout) {
        case eShaderReadOnlyOptimal:       return { sampledStage, AccessMask::eShaderSampledRead };
        case eDepthStencilReadOnlyOptimal: return { sampledStage | depthStage, AccessMask::eShaderSampledRead };
        default:                           return Image::getLayoutTransitionDstSync(newLayout);
    }
}