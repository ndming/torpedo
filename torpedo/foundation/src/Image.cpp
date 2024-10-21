#include "torpedo/foundation/Image.h"

std::unique_ptr<tpd::Image> tpd::Image::Builder::build(
    const vk::Device device,
    const ResourceAllocator& allocator,
    const ResourceType type,
    const vk::ImageCreateFlags flags) const
{
    switch (type) {
        case ResourceType::Dedicated: return buildDedicated(allocator, device, flags);
        default: throw std::runtime_error("Image::Builder - Unsupported resource type");
    }
}

std::unique_ptr<tpd::Image> tpd::Image::Builder::buildDedicated(
    const ResourceAllocator& allocator,
    const vk::Device device,
    const vk::ImageCreateFlags flags) const
{
    // Create a dedicated image
    auto allocation = VmaAllocation{};
    const auto imageInfo = populateImageCreateInfo(vk::ImageUsageFlagBits::eTransferDst, vk::ImageTiling::eOptimal, flags);
    const auto image = allocator.allocateDedicatedImage(imageInfo, &allocation);

    // Create an image view
    auto subresourceRange = vk::ImageSubresourceRange{};
    subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    subresourceRange.baseMipLevel = _viewBaseMipLevel;
    subresourceRange.levelCount = _mipLevels;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;
    auto viewInfo = vk::ImageViewCreateInfo{};
    viewInfo.image = image;
    viewInfo.viewType = _viewType;
    viewInfo.format = _format;
    viewInfo.subresourceRange = subresourceRange;
    const auto imageView = device.createImageView(viewInfo);

    return std::make_unique<Image>(image, imageView, allocation);
}

vk::ImageCreateInfo tpd::Image::Builder::populateImageCreateInfo(
    const vk::ImageUsageFlags internalUsage,
    const vk::ImageTiling tiling,
    const vk::ImageCreateFlags flags) const
{
    auto imageCreateInfo = vk::ImageCreateInfo{};
    imageCreateInfo.flags = flags;
    imageCreateInfo.imageType = _type;
    imageCreateInfo.format = _format;
    imageCreateInfo.extent = vk::Extent3D{ _width, _height, _depth };
    imageCreateInfo.mipLevels = _mipLevels;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = vk::SampleCountFlagBits::e1;
    imageCreateInfo.tiling = tiling;
    imageCreateInfo.usage = _usage | internalUsage;
    imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    imageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
    return imageCreateInfo;
}

void tpd::Image::transferImageData(
    const void* const data,
    const  std::uint32_t dataByteSize,
    const vk::ImageLayout finalLayout,
    const ResourceAllocator& allocator,
    const std::function<void(vk::ImageLayout, vk::ImageLayout, vk::Image)>& onLayoutTransition,
    const std::function<void(vk::Buffer, vk::Image)>& onBufferToImageCopy) const
{
    auto stagingAllocation = VmaAllocation{};
    const auto stagingBuffer = allocator.allocateStagingBuffer(dataByteSize, &stagingAllocation);
    allocator.mapAndCopyStagingBuffer(dataByteSize, data, stagingAllocation);

    using enum vk::ImageLayout;
    onLayoutTransition(eUndefined, eTransferDstOptimal, _image);
    onBufferToImageCopy(stagingBuffer, _image);
    onLayoutTransition(eTransferDstOptimal, finalLayout, _image);

    allocator.freeBuffer(stagingBuffer, stagingAllocation);
}

void tpd::Image::dispose(const vk::Device device, const ResourceAllocator& allocator) noexcept {
    device.destroyImageView(_imageView);
    allocator.freeImage(_image, _allocation);
    _allocation = nullptr;
}
