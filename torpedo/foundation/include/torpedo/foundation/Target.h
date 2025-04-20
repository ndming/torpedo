#pragma once

#include "torpedo/foundation/Image.h"

namespace tpd {
    class Target final : public Image {
    public:
        Target() noexcept = default;
        Target(vk::Format format, vk::Image image, VmaAllocation allocation) noexcept;

        void recordSrcImageCopy(vk::CommandBuffer cmd, vk::Image srcImage, vk::Extent3D extent) const noexcept;
        void recordDstImageCopy(vk::CommandBuffer cmd, vk::Image dstImage, vk::Extent3D extent) const noexcept;

        [[nodiscard]] SyncPoint getLayoutTransitionSrcSync(vk::ImageLayout oldLayout) const noexcept override;
        [[nodiscard]] SyncPoint getLayoutTransitionDstSync(vk::ImageLayout newLayout) const noexcept override;
    };
} // namespace tpd

inline tpd::Target::Target(const vk::Format format, const vk::Image image, VmaAllocation allocation) noexcept
    : Image{ format, image, allocation } {}

inline tpd::SyncPoint tpd::Target::getLayoutTransitionSrcSync(const vk::ImageLayout oldLayout) const noexcept {
    using PipelineStage = vk::PipelineStageFlagBits2;
    using AccessMask    = vk::AccessFlagBits2;

    // Override to provide a more relax transition point, given that Target is designed mainly for compute write
    using enum vk::ImageLayout;
    switch (oldLayout) {
        case eGeneral: return { PipelineStage::eComputeShader, AccessMask::eShaderStorageWrite };
        default:       return Image::getLayoutTransitionSrcSync(oldLayout);
    }
}

inline tpd::SyncPoint tpd::Target::getLayoutTransitionDstSync(const vk::ImageLayout newLayout) const noexcept {
    using PipelineStage = vk::PipelineStageFlagBits2;
    using AccessMask    = vk::AccessFlagBits2;

    // Override to provide a more relax transition point, given that Target is designed mainly for compute write
    using enum vk::ImageLayout;
    switch (newLayout) {
        case eGeneral: return { PipelineStage::eComputeShader, AccessMask::eShaderStorageWrite };
        default:       return Image::getLayoutTransitionDstSync(newLayout);
    }
}
