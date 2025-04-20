#include "torpedo/foundation/Texture.h"
#include "torpedo/foundation/ImageUtils.h"

vk::ImageAspectFlags tpd::Texture::getAspectMask() const noexcept {
    using enum vk::ImageAspectFlagBits;
    return isDepth() ? isStencil() ? eStencil | eDepth : eDepth : eColor;
}

bool tpd::Texture::isDepth() const noexcept {
    using enum vk::Format;
    return _format == eD16Unorm        ||
           _format == eD16UnormS8Uint  ||
           _format == eD24UnormS8Uint  ||
           _format == eD32Sfloat       ||
           _format == eD32SfloatS8Uint ||
           _format == eX8D24UnormPack32;
}

bool tpd::Texture::isStencil() const noexcept {
    using enum vk::Format;
    return _format == eS8Uint          ||
           _format == eD16UnormS8Uint  ||
           _format == eD24UnormS8Uint  ||
           _format == eD32SfloatS8Uint;
}

void tpd::Texture::recordStagingCopy(
    const vk::CommandBuffer cmd,
    const vk::Buffer stagingBuffer,
    const vk::Extent3D extent,
    const uint32_t mipLevel) const noexcept
{
    auto copyRegion = vk::BufferImageCopy2{};
    copyRegion.bufferOffset = 0;
    copyRegion.imageOffset = vk::Offset3D{ 0, 0, 0 };
    copyRegion.imageExtent = extent;
    copyRegion.imageSubresource = { getAspectMask(), mipLevel, 0 , 1 };
    // Data in the buffer is tightly packed
    copyRegion.bufferRowLength = 0;
    copyRegion.bufferImageHeight = 0;

    const auto copyInfo = vk::CopyBufferToImageInfo2{}
        .setSrcBuffer(stagingBuffer)
        .setDstImage(_resource)
        .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
        .setRegions(copyRegion);
    cmd.copyBufferToImage2(copyInfo);
}

void tpd::Texture::recordMipGen(
    const vk::CommandBuffer cmd,
    const vk::PhysicalDevice physicalDevice,
    const vk::Extent2D extent,
    const uint32_t mipCount,
    const vk::ImageLayout dstLayout) const
{
    // Make sure the physical device supports linear blit on the current image format
    using enum vk::FormatFeatureFlagBits;
    if (!(physicalDevice.getFormatProperties(_format).optimalTilingFeatures & eSampledImageFilterLinear)) [[unlikely]] {
        throw std::runtime_error("Texture - The current image format does not support linear blit: " + utils::toString(_format));
    }

    using AccessMask = vk::AccessFlagBits2;
    using StageMask  = vk::PipelineStageFlagBits2;

    // Reuse this barrier for all mip levels
    auto barrier = vk::ImageMemoryBarrier2{};
    barrier.image = _resource;
    barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.subresourceRange.aspectMask = getAspectMask();
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1; // gen one level at a time
    // It's almost always the case the src stage prior to mip generation is transfer
    // since mips are likely to be generated right after uploading data to this image
    barrier.srcStageMask = StageMask::eTransfer;

    // Other reusable values
    const auto dependency = vk::DependencyInfo{}
        .setImageMemoryBarrierCount(1)
        .setPImageMemoryBarriers(&barrier);
    const auto aspectMask = getAspectMask();

    const auto [dstStage, dstAccess] = getLayoutTransitionDstSync(dstLayout);

    auto mipWidth  = static_cast<int32_t>(extent.width);
    auto mipHeight = static_cast<int32_t>(extent.height);

    // At each iteration, we perform mip generation with vkCmdBlitImage from level i - 1 to i
    // For the sake of blit performance, src and dst level should be in transfer read and write, respectively
    // At the beginning of each iteration, level i - 1 (presumably, though very likely) has transfer dst layout
    // At the end of each iteration, level i - 1 has shader read layout
    for (uint32_t i = 1; i < mipCount; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        // We could relax the source access mask for level 0, but such an optimization is redundant
        barrier.srcAccessMask = AccessMask::eTransferWrite; // wait until mip level i - 1 done receiving data (from level i - 2)
        barrier.dstAccessMask = AccessMask::eTransferRead;  // we're about read and blit level i - 1 to level i
        barrier.dstStageMask  = StageMask::eTransfer; // this value has been changed at the end of the previous loop
        // Data at mip level i - 1 is now ready to be read into level i
        cmd.pipelineBarrier2(dependency);

        auto blitRegion = vk::ImageBlit2{};
        blitRegion.srcOffsets[0] = vk::Offset3D{ 0, 0, 0 };
        blitRegion.srcOffsets[1] = vk::Offset3D{ mipWidth, mipHeight, 1 };
        blitRegion.dstOffsets[0] = vk::Offset3D{ 0, 0, 0 };
        blitRegion.dstOffsets[1] = vk::Offset3D{ mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blitRegion.srcSubresource.aspectMask = aspectMask;
        blitRegion.dstSubresource.aspectMask = aspectMask;
        blitRegion.srcSubresource.mipLevel = i - 1;
        blitRegion.dstSubresource.mipLevel = i;
        blitRegion.srcSubresource.baseArrayLayer = 0;
        blitRegion.dstSubresource.baseArrayLayer = 0;
        blitRegion.srcSubresource.layerCount = 1;
        blitRegion.dstSubresource.layerCount = 1;

        const auto blitInfo = vk::BlitImageInfo2{}
            .setSrcImage(_resource).setDstImage(_resource)
            .setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
            .setFilter(vk::Filter::eLinear)
            .setRegions(blitRegion);
        // Data from level i - 1 has now been linearly down sampled and transferred to level i
        cmd.blitImage2(blitInfo);

        // Transition the layout at level i - 1 to the final layout
        barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = AccessMask::eTransferRead; // wait until blit has read all data from level i - 1
        barrier.dstStageMask = dstStage;
        barrier.dstAccessMask = dstAccess;
        // Layout at mip level i - 1 is now ready to be sampled from shaders
        cmd.pipelineBarrier2(dependency);

        // Halve the resolution and go to the next mip level
        if (mipWidth  > 1) mipWidth  >>= 1;
        if (mipHeight > 1) mipHeight >>= 1;
    }

    // The last level just received its blit data, no transition was done to it
    barrier.subresourceRange.baseMipLevel = mipCount - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = AccessMask::eTransferWrite;
    // The texture is (presumably) about to be consumed by one of these shaders
    barrier.dstStageMask = dstStage;
    barrier.dstAccessMask = dstAccess;
    cmd.pipelineBarrier2(dependency);
}

void tpd::Texture::recordReleaseForMipGen(
    const vk::CommandBuffer cmd,
    const uint32_t transferFamily,
    const uint32_t graphicsFamily) const noexcept
{
    constexpr auto srcSync = SyncPoint{ vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite };
    recordOwnershipRelease(cmd, transferFamily, graphicsFamily, srcSync, 0, 1); // only maintain the base mip content
}

void tpd::Texture::recordAcquireForMipGen(
    const vk::CommandBuffer cmd,
    const uint32_t transferFamily,
    const uint32_t graphicsFamily) const noexcept
{
    // Ensure the transferred memory (of the base mip level) is visible for blit
    constexpr auto dstSync = SyncPoint{ vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferRead };
    recordOwnershipAcquire(cmd, transferFamily, graphicsFamily, dstSync, 0, 1); // only maintain the base mip content
}
