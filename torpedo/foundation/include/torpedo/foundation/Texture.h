#pragma once

#include "torpedo/foundation/Image.h"
#include "torpedo/foundation/SyncResource.h"

namespace tpd {
    class Texture final : public Image, public SyncResource {
    public:
        class Builder {
        public:
            Builder& extent(uint32_t width, uint32_t height) noexcept;
            Builder& format(vk::Format format) noexcept;

            Builder& initialLayout(vk::ImageLayout layout) noexcept;
            Builder& mipLevelCount(uint32_t levels) noexcept;

            [[nodiscard]] Texture build(const DeviceAllocator& allocator) const;

            [[nodiscard]] std::unique_ptr<Texture, Deleter<Texture>> build(
                const DeviceAllocator& allocator, std::pmr::memory_resource* pool) const;

        private:
            [[nodiscard]] vk::Image createImage(const DeviceAllocator& allocator, VmaAllocation* allocation) const;

            uint32_t _w{ 0 };
            uint32_t _h{ 0 };
            vk::Format _format{ vk::Format::eR8G8B8A8Srgb };
            vk::ImageLayout _layout{ vk::ImageLayout::eUndefined };
            uint32_t _mipLevelCount{ 1 };
        };

        Texture(vk::Extent2D size, uint32_t mipLevelCount,
            vk::Image image, vk::ImageLayout layout, vk::Format format,
            VmaAllocation allocation, const DeviceAllocator& allocator);

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        [[nodiscard]] vk::ImageAspectFlags getAspectMask() const noexcept override;

        [[nodiscard]] bool isDepth() const noexcept;
        [[nodiscard]] bool isStencil() const noexcept;

        void recordBufferTransfer(vk::CommandBuffer cmd, vk::Buffer stagingBuffer, uint32_t mipLevel = 0) const noexcept;
        void recordMipsGeneration(vk::CommandBuffer cmd, vk::PhysicalDevice physicalDevice);

        void recordOwnershipRelease(vk::CommandBuffer cmd, uint32_t srcFamilyIndex, uint32_t dstFamilyIndex) const noexcept override;
        void recordOwnershipAcquire(vk::CommandBuffer cmd, uint32_t srcFamilyIndex, uint32_t dstFamilyIndex) const noexcept override;

        void recordOwnershipRelease(vk::CommandBuffer cmd, uint32_t srcFamilyIndex, uint32_t dstFamilyIndex, vk::ImageLayout finalLayout) const noexcept;
        void recordOwnershipAcquire(vk::CommandBuffer cmd, uint32_t srcFamilyIndex, uint32_t dstFamilyIndex, vk::ImageLayout finalLayout) noexcept;

        [[nodiscard]] std::pair<vk::PipelineStageFlags2, vk::AccessFlags2> getDstSync(vk::ImageLayout layout) const override;

        [[nodiscard]] vk::Extent2D getSize() const noexcept;
        [[nodiscard]] uint32_t getMipLevelCount() const noexcept override;

    private:
        vk::Extent2D _size;       // width and height in pixels
        uint32_t _mipLevelCount;  // including the base mip
    };
}

// =========================== //
// INLINE FUNCTION DEFINITIONS //
// =========================== //

inline tpd::Texture::Builder& tpd::Texture::Builder::extent(const uint32_t width, const uint32_t height) noexcept {
    _w = width;
    _h = height;
    return *this;
}

inline tpd::Texture::Builder& tpd::Texture::Builder::format(const vk::Format format) noexcept {
    _format = format;
    return *this;
}

inline tpd::Texture::Builder& tpd::Texture::Builder::initialLayout(const vk::ImageLayout layout) noexcept {
    _layout = layout;
    return *this;
}

inline tpd::Texture::Builder& tpd::Texture::Builder::mipLevelCount(const uint32_t levels) noexcept {
    _mipLevelCount = levels > 0 ? levels : static_cast<uint32_t>(std::floor(std::log2(std::max(_w, _h)))) + 1;
    return *this;
}

inline tpd::Texture::Texture(
    const vk::Extent2D size, const uint32_t mipLevelCount,
    const vk::Image image, const vk::ImageLayout layout, const vk::Format format,
    VmaAllocation allocation, const DeviceAllocator& allocator)
    : Image{ image, layout, format, allocation, allocator }
    , _size{ size }
    , _mipLevelCount{ mipLevelCount } {
}

inline vk::Extent2D tpd::Texture::getSize() const noexcept {
    return _size;
}

inline uint32_t tpd::Texture::getMipLevelCount() const noexcept {
    return _mipLevelCount;
}
