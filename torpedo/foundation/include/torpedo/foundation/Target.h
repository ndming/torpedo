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
    // When a render Target is first used, it needs a layout transition from undefined, and for the rest of the program,
    // the intended oldLayout to transition from will be general and transfer src. We keep it simple by treating layout
    // transition from undefined the same as transfer src. This way, downstream won't have to perform an additional
    // transition from undefined to transfer src and still keep the drawing logic consistent.
    using enum vk::ImageLayout;
    switch (oldLayout) {
        case eUndefined: return Image::getLayoutTransitionSrcSync(eTransferSrcOptimal);
        case eGeneral:   return { PipelineStage::eComputeShader, AccessMask::eShaderStorageWrite };
        default:         return Image::getLayoutTransitionSrcSync(oldLayout);
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
