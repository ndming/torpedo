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
    return foundation::make_unique<StorageImage>(
        pool, image, _layout, _format, allocation, allocator, _data, _dataSize);
}