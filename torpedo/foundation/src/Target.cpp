#include "torpedo/foundation/Target.h"

void tpd::Target::recordSrcImageCopy(const vk::CommandBuffer cmd, const vk::Image srcImage, const vk::Extent3D extent) const noexcept {
    auto copyRegion = vk::ImageCopy2{};
    copyRegion.srcOffset = vk::Offset3D{ 0, 0, 0 };
    copyRegion.dstOffset = vk::Offset3D{ 0, 0, 0 };
    copyRegion.extent = extent;
    // VK_REMAINING_ARRAY_LAYERS is only valid if the maintenance5 feature is enabled
    copyRegion.srcSubresource = { getAspectMask(), /* mip level */ 0, 0, 1 };
    copyRegion.dstSubresource = { getAspectMask(), /* mip level */ 0, 0, 1 };

    const auto copyInfo = vk::CopyImageInfo2{}
        .setSrcImage(srcImage)
        .setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal)
        .setDstImage(_resource)
        .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
        .setRegions(copyRegion);
    cmd.copyImage2(copyInfo);
}

void tpd::Target::recordDstImageCopy(const vk::CommandBuffer cmd, const vk::Image dstImage, const vk::Extent3D extent) const noexcept {
    auto copyRegion = vk::ImageCopy2{};
    copyRegion.srcOffset = vk::Offset3D{ 0, 0, 0 };
    copyRegion.dstOffset = vk::Offset3D{ 0, 0, 0 };
    copyRegion.extent = extent;
    // VK_REMAINING_ARRAY_LAYERS is only valid if the maintenance5 feature is enabled
    copyRegion.srcSubresource = { getAspectMask(), /* mip level */ 0, 0, 1 };
    copyRegion.dstSubresource = { getAspectMask(), /* mip level */ 0, 0, 1 };

    const auto copyInfo = vk::CopyImageInfo2{}
        .setSrcImage(_resource)
        .setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal)
        .setDstImage(dstImage)
        .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
        .setRegions(copyRegion);
    cmd.copyImage2(copyInfo);
}
