#pragma once

#include "torpedo/foundation/Image.h"
#include "torpedo/foundation/SyncResource.h"

namespace tpd {
    class StorageImage final : public Image, public SyncResource {
    public:
        class Builder {
        public:
            Builder& extent(uint32_t width, uint32_t height, uint32_t depth = 1) noexcept;
            Builder& format(vk::Format format) noexcept;

            Builder& usage(vk::ImageUsageFlags usage) noexcept;
            Builder& initialLayout(vk::ImageLayout layout) noexcept;

            Builder& syncData(const void* data, uint32_t size) noexcept;

            [[nodiscard]] StorageImage build(const DeviceAllocator& allocator) const;

            [[nodiscard]] std::unique_ptr<StorageImage, Deleter<StorageImage>> build(
                const DeviceAllocator& allocator, std::pmr::memory_resource* pool) const;

        private:
            [[nodiscard]] vk::Image createImage(const DeviceAllocator& allocator, VmaAllocation* allocation) const;

            uint32_t _w{ 0 };
            uint32_t _h{ 0 };
            uint32_t _d{ 1 };

            vk::ImageUsageFlags _usage{};
            vk::Format _format{ vk::Format::eUndefined };
            vk::ImageLayout _layout{ vk::ImageLayout::eUndefined };

            const void* _data{ nullptr };
            uint32_t _dataSize{ 0 };
        };

        StorageImage(
            vk::Image image, vk::ImageLayout layout, vk::Format format,
            VmaAllocation allocation, const DeviceAllocator& allocator,
            const void* data = nullptr, uint32_t dataByteSize = 0);

        [[nodiscard]] vk::ImageAspectFlags getAspectMask() const noexcept override;

        void recordOwnershipRelease(vk::CommandBuffer cmd, uint32_t srcFamilyIndex, uint32_t dstFamilyIndex) const noexcept override;
        void recordOwnershipAcquire(vk::CommandBuffer cmd, uint32_t srcFamilyIndex, uint32_t dstFamilyIndex) const noexcept override;
    };
}  // namespace tpd

inline tpd::StorageImage::Builder& tpd::StorageImage::Builder::extent(const uint32_t width, const uint32_t height, const uint32_t depth) noexcept {
    _w = width;
    _h = height;
    _d = depth;
    return *this;
}

inline tpd::StorageImage::Builder& tpd::StorageImage::Builder::format(const vk::Format format) noexcept {
    _format = format;
    return *this;
}

inline tpd::StorageImage::Builder& tpd::StorageImage::Builder::usage(const vk::ImageUsageFlags usage) noexcept {
    _usage = usage;
    return *this;
}

inline tpd::StorageImage::Builder& tpd::StorageImage::Builder::initialLayout(const vk::ImageLayout layout) noexcept {
    _layout = layout;
    return *this;
}

inline tpd::StorageImage::Builder& tpd::StorageImage::Builder::syncData(const void* data, const uint32_t size) noexcept {
    _data = data;
    _dataSize = size;
    return *this;
}

inline tpd::StorageImage::StorageImage(
    const vk::Image image,
    const vk::ImageLayout layout,
    const vk::Format format,
    VmaAllocation allocation,
    const DeviceAllocator& allocator,
    const void* data,
    const uint32_t dataByteSize)
    : Image{ image, layout, format, allocation, allocator }
    , SyncResource{ data, dataByteSize } {
}
