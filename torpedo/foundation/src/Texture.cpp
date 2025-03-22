#include "torpedo/foundation/Texture.h"
#include "torpedo/foundation/ImageUtils.h"

tpd::Texture tpd::Texture::Builder::build(const DeviceAllocator& allocator) const {
    auto allocation  = VmaAllocation{};
    const auto image = createImage(allocator, &allocation);
    return Texture{ vk::Extent2D{ _w, _h }, _mipLevelCount, image, _layout, _format, allocation, allocator, _data, _dataSize };
}

std::unique_ptr<tpd::Texture, tpd::Deleter<tpd::Texture>> tpd::Texture::Builder::build(
    const DeviceAllocator& allocator,
    std::pmr::memory_resource* pool) const
{
    auto allocation  = VmaAllocation{};
    const auto image = createImage(allocator, &allocation);
    return foundation::make_unique<Texture>(
        pool, vk::Extent2D{ _w, _h }, _mipLevelCount, image, _layout, _format, allocation, allocator, _data, _dataSize);
}

vk::Image tpd::Texture::Builder::createImage(const DeviceAllocator& allocator, VmaAllocation* allocation) const {
    if (_w == 0 || _h == 0) [[unlikely]] {
        throw std::runtime_error(
            "Texture::Builder - Image is being built with 0 dimensions: "
            "did you forget to call Texture::Builder::extent()?");
    }

    if (_format == vk::Format::eUndefined) [[unlikely]] {
        throw std::runtime_error(
            "Texture::Builder - Image cannot be initialized with an undefined format (except in very rare cases which we don't support): "
            "did you forget to call Texture::Builder::format()?");
    }

    // A Texture must support shader sampling and data transfer from a staging buffer by default.
    // If using mips, it must also be a transfer source for blit.
    using enum vk::ImageUsageFlagBits;
    auto usage = eSampled | eTransferDst;
    if (_mipLevelCount > 1) {
        usage |= eTransferSrc;
    }

    const auto imageCreateInfo = vk::ImageCreateInfo{}
        .setImageType(vk::ImageType::e2D)
        .setFormat(_format)
        .setExtent(vk::Extent3D{ _w, _h, 1 })
        .setMipLevels(_mipLevelCount)
        .setTiling(vk::ImageTiling::eOptimal)     // for efficient access from shaders
        .setSamples(vk::SampleCountFlagBits::e1)  // sampled textures don't need multi-sampling
        .setUsage(usage)
        .setSharingMode(vk::SharingMode::eExclusive)
        .setInitialLayout(_layout)  // must not be undefined, except working with Android builds
        .setArrayLayers(1);         // multi-view textures are not supported at the moment
    return allocator.allocateDeviceImage(imageCreateInfo, allocation);
}

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

void tpd::Texture::recordBufferTransfer(
    const vk::CommandBuffer cmd,
    const vk::Buffer stagingBuffer,
    const uint32_t mipLevel) const noexcept
{
    // Transfer to the whole image at mip level 0
    const auto copyRegion = vk::BufferImageCopy2{}
        .setBufferOffset(0)
        .setImageOffset({ 0, 0, 0 })
        .setBufferRowLength(0)    // tightly packed
        .setBufferImageHeight(0)  // tightly packed
        .setImageExtent({ _dims.width, _dims.height, 1 })
        .setImageSubresource({ getAspectMask(), mipLevel, 0 , 1 });

    const auto copyInfo = vk::CopyBufferToImageInfo2{}
        .setSrcBuffer(stagingBuffer)
        .setDstImage(_image)
        .setDstImageLayout(_layout)
        .setRegions(copyRegion);

    cmd.copyBufferToImage2(copyInfo);
}

void tpd::Texture::recordMipsGeneration(const vk::CommandBuffer cmd, const vk::PhysicalDevice physicalDevice) {
    // Make sure the physical device supports linear blit on the current image format
    using enum vk::FormatFeatureFlagBits;
    if (!(physicalDevice.getFormatProperties(_format).optimalTilingFeatures & eSampledImageFilterLinear)) [[unlikely]] {
        throw std::runtime_error("Texture - The current image format does not support linear blit: " + foundation::toString(_format));
    }

    using AccessMask = vk::AccessFlagBits2;
    using StageMask  = vk::PipelineStageFlagBits2;

    // Reuse this barrier for all mip levels
    auto barrier = vk::ImageMemoryBarrier2{};
    barrier.image = _image;
    barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.subresourceRange.aspectMask = getAspectMask();
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;  // gen one level at a time
    // It's almost always the case the src stage prior to mip generation is transfer
    // since mips are likely to be generated right after uploading data to this image
    barrier.srcStageMask = StageMask::eTransfer;

    // Other reusable values
    const auto dependency = vk::DependencyInfo{}
        .setImageMemoryBarrierCount(1)
        .setPImageMemoryBarriers(&barrier);
    const auto aspectMask = getAspectMask();

    auto mipWidth  = static_cast<int32_t>(_dims.width);
    auto mipHeight = static_cast<int32_t>(_dims.height);

    // At each iteration, we perform mip generation with vkCmdBlitImage from level i - 1 to i
    // For the sake of blit performance, src and dst level should be in transfer read and write, respectively
    // At the beginning of each iteration, level i - 1 (presumably, though very likely) has transfer dst layout
    // At the end of each iteration, level i - 1 has shader read layout
    for (uint32_t i = 1; i < _mipLevelsCount; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = _layout;  // the layout prior to mip generation, likely to be transfer dst (how could it not?)
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        // We could relax the source access mask for level 0, but such an optimization is redundant
        barrier.srcAccessMask = AccessMask::eTransferWrite;  // wait until mip level i - 1 done receiving data (from level i - 2)
        barrier.dstAccessMask = AccessMask::eTransferRead;   // we're about read and blit level i - 1 to level i
        barrier.dstStageMask  = StageMask::eTransfer;        // this value has been changed at the end of the previous loop
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
            .setSrcImage(_image).setDstImage(_image)
            .setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setDstImageLayout(_layout)  // likely to be transfer dst, but would Vulkan complain if it's not?
            .setFilter(vk::Filter::eLinear)
            .setRegions(blitRegion);
        // Data from level i - 1 has now been linearly down sampled and transferred to level i
        cmd.blitImage2(blitInfo);

        // Transition the layout at level i - 1 to the final layout
        barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = AccessMask::eTransferRead;       // wait until blit has read all data from level i - 1
        barrier.dstAccessMask = AccessMask::eShaderSampledRead;  // why would anyone gen mip for non-sampled resource?
        // The texture is (presumably) about to be consumed by one of these shaders
        barrier.dstStageMask = StageMask::eVertexShader | StageMask::eFragmentShader | StageMask::eComputeShader;
        // Layout at mip level i - 1 is now ready to be sampled from shaders
        cmd.pipelineBarrier2(dependency);

        // Halve the resolution and go to the next mip level
        if (mipWidth  > 1) mipWidth  >>= 1;
        if (mipHeight > 1) mipHeight >>= 1;
    }

    // Transition the last mip level's layout
    barrier.subresourceRange.baseMipLevel = _mipLevelsCount - 1;
    barrier.oldLayout = _layout;  // the last level just received its blit data, no transition was done to it
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = AccessMask::eTransferWrite;
    barrier.dstAccessMask = AccessMask::eShaderSampledRead;
    // This value wasn't changed, but it couldn't hurt to be explicit
    barrier.dstStageMask = StageMask::eVertexShader | StageMask::eFragmentShader | StageMask::eComputeShader;
    cmd.pipelineBarrier2(dependency);

    // The whole image and its mips are now in shader read layout
    _layout = vk::ImageLayout::eShaderReadOnlyOptimal;
}

void tpd::Texture::recordOwnershipRelease(
    const vk::CommandBuffer cmd,
    const uint32_t srcFamilyIndex,
    const uint32_t dstFamilyIndex) const noexcept
{
    auto barrier = vk::ImageMemoryBarrier2{};
    barrier.image = _image;
    barrier.subresourceRange = { getAspectMask(), 0, _mipLevelsCount, 0, 1 };
    barrier.srcQueueFamilyIndex = srcFamilyIndex;
    barrier.dstQueueFamilyIndex = dstFamilyIndex;
    barrier.srcStageMask  = vk::PipelineStageFlagBits2::eTransfer;  // releasing after image uploading
    barrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;    // ensure memory write is available

    // TODO: consider VK_DEPENDENCY_QUEUE_FAMILY_OWNERSHIP_TRANSFER_USE_ALL_STAGES_BIT_KHR to avoid full pipeline stalls
    const auto dependency = vk::DependencyInfo{}.setImageMemoryBarriers(barrier);
    cmd.pipelineBarrier2(dependency);
}

void tpd::Texture::recordOwnershipAcquire(
    const vk::CommandBuffer cmd,
    const uint32_t srcFamilyIndex,
    const uint32_t dstFamilyIndex) const noexcept
{
    auto barrier = vk::ImageMemoryBarrier2{};
    barrier.image = _image;
    barrier.subresourceRange = { getAspectMask(), 0, _mipLevelsCount, 0, 1 };
    barrier.srcQueueFamilyIndex = srcFamilyIndex;
    barrier.dstQueueFamilyIndex = dstFamilyIndex;
    // Without specifying the final layout, this overload assumes
    // the texture is about to undergo mip generation
    barrier.dstStageMask  = vk::PipelineStageFlagBits2::eTransfer;  // blit happens at transfer
    barrier.dstAccessMask = vk::AccessFlagBits2::eTransferRead;     // ensure memory is visible for blit

    // TODO: consider VK_DEPENDENCY_QUEUE_FAMILY_OWNERSHIP_TRANSFER_USE_ALL_STAGES_BIT_KHR to avoid full pipeline stalls
    const auto dependency = vk::DependencyInfo{}.setImageMemoryBarriers(barrier);
    cmd.pipelineBarrier2(dependency);
}

void tpd::Texture::recordOwnershipRelease(
    const vk::CommandBuffer cmd,
    const uint32_t srcFamilyIndex,
    const uint32_t dstFamilyIndex,
    const vk::ImageLayout finalLayout) const noexcept
{
    auto barrier = vk::ImageMemoryBarrier2{};
    barrier.image = _image;
    barrier.subresourceRange = { getAspectMask(), 0, _mipLevelsCount, 0, 1 };
    barrier.srcQueueFamilyIndex = srcFamilyIndex;
    barrier.dstQueueFamilyIndex = dstFamilyIndex;
    barrier.oldLayout = _layout;
    barrier.newLayout = finalLayout;
    barrier.srcStageMask  = vk::PipelineStageFlagBits2::eTransfer;  // releasing after image uploading
    barrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;    // ensure memory write is available

    // TODO: consider VK_DEPENDENCY_QUEUE_FAMILY_OWNERSHIP_TRANSFER_USE_ALL_STAGES_BIT_KHR to avoid full pipeline stalls
    const auto dependency = vk::DependencyInfo{}.setImageMemoryBarriers(barrier);
    cmd.pipelineBarrier2(dependency);
}

void tpd::Texture::recordOwnershipAcquire(
    const vk::CommandBuffer cmd,
    const uint32_t srcFamilyIndex,
    const uint32_t dstFamilyIndex,
    const vk::ImageLayout finalLayout) noexcept
{
    const auto [dstStageMask, dstAccessMask] = getDstSync(finalLayout);
    auto barrier = vk::ImageMemoryBarrier2{};
    barrier.image = _image;
    barrier.subresourceRange = { getAspectMask(), 0, _mipLevelsCount, 0, 1 };
    barrier.srcQueueFamilyIndex = srcFamilyIndex;
    barrier.dstQueueFamilyIndex = dstFamilyIndex;
    barrier.oldLayout = _layout;
    barrier.newLayout = finalLayout;
    barrier.dstStageMask  = dstStageMask;
    barrier.dstAccessMask = dstAccessMask;

    // TODO: consider VK_DEPENDENCY_QUEUE_FAMILY_OWNERSHIP_TRANSFER_USE_ALL_STAGES_BIT_KHR to avoid full pipeline stalls
    const auto dependency = vk::DependencyInfo{}.setImageMemoryBarriers(barrier);
    cmd.pipelineBarrier2(dependency);

    // The layout transition during queue ownership transfer happens-after the release operation and
    // happens-before the acquire operation. This seems like the best place to update the layout state.
    _layout = finalLayout;
}

std::pair<vk::PipelineStageFlags2, vk::AccessFlags2> tpd::Texture::getDstSync(const vk::ImageLayout layout) const {
    using PipelineStage = vk::PipelineStageFlagBits2;
    using AccessMask    = vk::AccessFlagBits2;

    constexpr auto depthStage   = PipelineStage::eEarlyFragmentTests | PipelineStage::eLateFragmentTests;
    constexpr auto sampledStage = PipelineStage::eVertexShader | PipelineStage::eFragmentShader | PipelineStage::eComputeShader;

    using enum vk::ImageLayout;
    switch (layout) {
        case eShaderReadOnlyOptimal:       return std::make_pair(sampledStage, AccessMask::eShaderSampledRead);
        case eDepthStencilReadOnlyOptimal: return std::make_pair(sampledStage | depthStage, AccessMask::eShaderSampledRead);
        default:                           return Image::getDstSync(layout);
    }
}
