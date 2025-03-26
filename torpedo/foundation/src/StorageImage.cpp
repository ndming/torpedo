#include "torpedo/foundation/StorageImage.h"

tpd::StorageImage tpd::StorageImage::Builder::build(const DeviceAllocator& allocator) const {
    auto allocation  = VmaAllocation{};
    const auto image = createImage(allocator, &allocation);
    return StorageImage{ image, _layout, _format, allocation, allocator, _data, _dataSize };
}

std::unique_ptr<tpd::StorageImage, tpd::Deleter<tpd::StorageImage>> tpd::StorageImage::Builder::build(
    const DeviceAllocator& allocator,
    std::pmr::memory_resource* pool) const
{
    auto allocation  = VmaAllocation{};
    const auto image = createImage(allocator, &allocation);
    return foundation::make_unique<StorageImage>(pool, image, _layout, _format, allocation, allocator, _data, _dataSize);
}

vk::Image tpd::StorageImage::Builder::createImage(const DeviceAllocator& allocator, VmaAllocation* allocation) const {
    if (_w == 0 || _h == 0 || _d == 0) [[unlikely]] {
        throw std::runtime_error(
            "StorageImage::Builder - Image is being built with 0 dimensions: "
            "did you forget to call StorageImage::Builder::extent()?");
    }

    if (_format == vk::Format::eUndefined) [[unlikely]] {
        throw std::runtime_error(
            "StorageImage::Builder - Image cannot be initialized with an undefined format (except in very rare cases "
            "which we don't support): did you forget to call StorageImage::Builder::format()?");
    }

    const auto imageCreateInfo = vk::ImageCreateInfo{}
        .setImageType(_d > 1 ? vk::ImageType::e3D : _h > 1 ? vk::ImageType::e2D : vk::ImageType::e1D)
        .setExtent(vk::Extent3D{ _w, _h, _d })
        .setTiling(vk::ImageTiling::eOptimal)     // for efficient access from shaders
        .setSamples(vk::SampleCountFlagBits::e1)  // storage images don't need multi-sampling
        .setUsage(_usage | vk::ImageUsageFlagBits::eStorage)
        .setSharingMode(vk::SharingMode::eExclusive)
        .setInitialLayout(_layout)
        .setMipLevels(1)       // mipmaps are only applicable for sampled images (textures)
        .setFormat(_format)    // must not be undefined, except working with Android builds
        .setArrayLayers(1);    // multi-view textures are not supported at the moment
    return allocator.allocateDeviceImage(imageCreateInfo, allocation);
}
