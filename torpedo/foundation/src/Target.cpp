#include "torpedo/foundation/Target.h"

tpd::Target tpd::Target::Builder::build(const DeviceAllocator& allocator) const {
    auto allocation = VmaAllocation{};
    const auto image = createImage(allocator, &allocation);
    return Target{ vk::Extent3D{ _w, _h, _d }, _aspects, _tiling, _mipLevels, 
        image, _layout, _format, allocation, allocator, _data, _dataSize };
}

std::unique_ptr<tpd::Target, tpd::Deleter<tpd::Target>>
tpd::Target::Builder::build(const DeviceAllocator& allocator, std::pmr::memory_resource* pool) const {
    auto allocation = VmaAllocation{};
    const auto image = createImage(allocator, &allocation);
    return foundation::make_unique<Target>(pool, vk::Extent3D{ _w, _h, _d }, _aspects, _tiling, _mipLevels, 
        image, _layout, _format, allocation, allocator, _data, _dataSize);
}

vk::Image tpd::Target::Builder::createImage(const DeviceAllocator& allocator, VmaAllocation* allocation) const {
    if (_w == 0 || _h == 0 || _d == 0) [[unlikely]] {
        throw std::runtime_error(
            "Target::Builder - Image is being built with 0 dimensions: "
            "did you forget to call Target::Builder::extent()?");
    }

    if (_format == vk::Format::eUndefined) [[unlikely]] {
        throw std::runtime_error(
            "Target::Builder - Image cannot be initialized with an undefined format (except in very rare cases "
            "which we don't support): did you forget to call Target::Builder::format()?");
    }

    if (_aspects == vk::ImageAspectFlagBits::eNone) [[unlikely]] {
        throw std::runtime_error(
            "Target::Builder - Image cannot be initialized with no aspects: "
            "did you forget to call Target::Builder::aspect()?");
    }

    const auto imageCreateInfo = vk::ImageCreateInfo{}
        .setImageType(_d > 1 ? vk::ImageType::e3D : _h > 1 ? vk::ImageType::e2D : vk::ImageType::e1D)
        .setExtent(vk::Extent3D{_w, _h, _d})
        .setUsage(_usage)
        .setTiling(_tiling)
        .setSamples(vk::SampleCountFlagBits::e1)  // only applicable for attachments
        .setSharingMode(vk::SharingMode::eExclusive)
        .setInitialLayout(_layout)
        .setMipLevels(_mipLevels)
        .setFormat(_format)  // must not be undefined, except when working with Android builds
        .setArrayLayers(1);  // multi-view textures are not supported at the moment
    return allocator.allocateDeviceImage(imageCreateInfo, allocation);
}