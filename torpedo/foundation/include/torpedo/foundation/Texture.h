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

            Builder& syncData(const void* data, uint32_t size) noexcept;

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

            const void* _data{ nullptr };
            uint32_t _dataSize{ 0 };
        };

        Texture(vk::Extent2D dims, uint32_t mipLevelsCount,
            vk::Image image, vk::ImageLayout layout, vk::Format format,
            VmaAllocation allocation, const DeviceAllocator& allocator,
            const void* data = nullptr, uint32_t dataByteSize = 0);

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

        [[nodiscard]] vk::Extent2D getDimensions() const noexcept;
        [[nodiscard]] uint32_t getMipLevelsCount() const noexcept override;

    private:
        vk::Extent2D _dims;        // width and height in pixels
        uint32_t _mipLevelsCount;  // including the base mip
    };
}

// INLINE FUNCTION DEFINITIONS
// ---------------------------

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

inline tpd::Texture::Builder& tpd::Texture::Builder::syncData(const void* data, const uint32_t size) noexcept {
    _data = data;
    _dataSize = size;
    return *this;
}

inline tpd::Texture::Texture(
    const vk::Extent2D dims, const uint32_t mipLevelsCount,
    const vk::Image image, const vk::ImageLayout layout, const vk::Format format,
    VmaAllocation allocation, const DeviceAllocator& allocator,
    const void* data, const uint32_t dataByteSize)
    : Image{ image, layout, format, allocation, allocator }
    , SyncResource{ data, dataByteSize }
    , _dims{ dims }
    , _mipLevelsCount{ mipLevelsCount } {
}

inline vk::Extent2D tpd::Texture::getDimensions() const noexcept {
    return _dims;
}

inline uint32_t tpd::Texture::getMipLevelsCount() const noexcept {
    return _mipLevelsCount;
}
