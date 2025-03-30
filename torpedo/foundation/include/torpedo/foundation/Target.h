#pragma once

#include "torpedo/foundation/Image.h"

namespace tpd {
    class Target final : public Image, public SyncResource {
    public:
        class Builder {
        public:
            Builder& usage(vk::ImageUsageFlags usage) noexcept;

            Builder& extent(uint32_t width, uint32_t height, uint32_t depth = 1) noexcept;
            Builder& extent(vk::Extent2D dims) noexcept;
            Builder& extent(vk::Extent3D dims) noexcept;
            Builder& format(vk::Format format) noexcept;

            Builder& aspect(vk::ImageAspectFlags aspects) noexcept;
            Builder& tiling(vk::ImageTiling tiling) noexcept;
            
            Builder& initialLayout(vk::ImageLayout layout) noexcept;
            Builder& syncData(const void* data, uint32_t size) noexcept;

            [[nodiscard]] Target build(const DeviceAllocator& allocator) const;

            [[nodiscard]] std::unique_ptr<Target, Deleter<Target>> build(
                const DeviceAllocator& allocator, std::pmr::memory_resource* pool) const;

        private:
            [[nodiscard]] vk::Image createImage(const DeviceAllocator& allocator, VmaAllocation* allocation) const;

            uint32_t _w{ 0 }, _h{ 0 }, _d{ 1 };
            vk::ImageUsageFlags _usage{};
            vk::Format _format{ vk::Format::eUndefined };
            vk::ImageAspectFlags _aspects{ vk::ImageAspectFlagBits::eNone };
            vk::ImageTiling _tiling{ vk::ImageTiling::eOptimal };
            vk::ImageLayout _layout{vk::ImageLayout::eUndefined};
            const void* _data{ nullptr };
            uint32_t _dataSize{ 0 };
        };

        Target(
            vk::Extent3D dims, vk::ImageAspectFlags aspects, vk::ImageTiling tiling,
            vk::Image image, vk::ImageLayout layout, vk::Format format, VmaAllocation allocation,
            const DeviceAllocator& allocator, const void* data = nullptr, uint32_t dataByteSize = 0);

        Target(Target&&) noexcept = default;
        Target& operator=(Target&&) noexcept = default;

        [[nodiscard]] vk::ImageAspectFlags getAspectMask() const noexcept override;

        [[nodiscard]] vk::Extent3D getPixelSize() const noexcept;
        [[nodiscard]] vk::ImageTiling getTiling() const noexcept;

        void recordOwnershipRelease(vk::CommandBuffer cmd, uint32_t srcFamilyIndex, uint32_t dstFamilyIndex) const noexcept;
        void recordOwnershipAcquire(vk::CommandBuffer cmd, uint32_t srcFamilyIndex, uint32_t dstFamilyIndex) noexcept;

        [[nodiscard]] SyncPoint getTransitionSrcPoint(vk::ImageLayout oldLayout) const override;
        [[nodiscard]] SyncPoint getTransitionDstPoint(vk::ImageLayout newLayout) const override;

        void recordImageCopyFrom(vk::Image srcImage, vk::CommandBuffer cmd) const;
        void recordImageCopyTo  (vk::Image dstImage, vk::CommandBuffer cmd) const;

    private:
        vk::Extent3D _dims;
        vk::ImageAspectFlags _aspects;
        vk::ImageTiling _tiling;
    };
}  // namespace tpd

inline tpd::Target::Builder& tpd::Target::Builder::usage(const vk::ImageUsageFlags usage) noexcept {
    _usage = usage;
    return *this;
}

inline tpd::Target::Builder& tpd::Target::Builder::extent(const uint32_t width, const uint32_t height, const uint32_t depth) noexcept {
    _w = width;
    _h = height;
    _d = depth;
    return *this;
}

inline tpd::Target::Builder& tpd::Target::Builder::extent(const vk::Extent2D dims) noexcept {
    return extent(dims.width, dims.height);
}

inline tpd::Target::Builder& tpd::Target::Builder::extent(const vk::Extent3D dims) noexcept {
    return extent(dims.width, dims.height, dims.depth);
}

inline tpd::Target::Builder& tpd::Target::Builder::format(const vk::Format format) noexcept {
    _format = format;
    return *this;
}

inline tpd::Target::Builder& tpd::Target::Builder::aspect(const vk::ImageAspectFlags aspects) noexcept {
    _aspects = aspects;
    return *this;
}

inline tpd::Target::Builder& tpd::Target::Builder::tiling(const vk::ImageTiling tiling) noexcept {
    _tiling = tiling;
    return *this;
}

inline tpd::Target::Builder& tpd::Target::Builder::initialLayout(const vk::ImageLayout layout) noexcept {
    _layout = layout;
    return *this;
}

inline tpd::Target::Builder& tpd::Target::Builder::syncData(const void* data, const uint32_t size) noexcept {
    _data = data;
    _dataSize = size;
    return *this;
}

inline tpd::Target::Target(
    const vk::Extent3D dims,
    const vk::ImageAspectFlags aspects,
    const vk::ImageTiling tiling,
    const vk::Image image,
    const vk::ImageLayout layout,
    const vk::Format format,
    VmaAllocation allocation,
    const DeviceAllocator& allocator, 
    const void* data, 
    const uint32_t dataByteSize)
    : Image{ image, layout, format, allocation, allocator }
    , SyncResource{ data, dataByteSize }
    , _dims{ dims }
    , _aspects{ aspects }
    , _tiling{ tiling }
{}

inline vk::ImageAspectFlags tpd::Target::getAspectMask() const noexcept {
    return _aspects;
}

inline vk::Extent3D tpd::Target::getPixelSize() const noexcept {
    return _dims;
}

inline vk::ImageTiling tpd::Target::getTiling() const noexcept {
    return _tiling;
}
