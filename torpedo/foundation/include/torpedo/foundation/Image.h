#pragma once

#include "torpedo/foundation/VmaUsage.h"

namespace tpd {
    class Image;

    template<typename T>
    concept ImageImpl = std::derived_from<T, Image> && std::is_final_v<T>;

    class Image : public OpaqueResource<vk::Image> {
    public:
        Image() noexcept = default;
        Image(vk::Format format, vk::Image image, VmaAllocation allocation) noexcept;

        Image(Image&&) noexcept = default;
        Image& operator=(Image&&) noexcept = default;

        [[nodiscard]] vk::ImageView createImageView(vk::ImageViewType type, vk::Device device, vk::ImageViewCreateFlags flags = {}) const;

        void recordLayoutTransition(
            vk::CommandBuffer cmd, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
            uint32_t baseMip = 0, uint32_t mipCount = vk::RemainingMipLevels) const noexcept;

        void recordOwnershipRelease(
            vk::CommandBuffer cmd, uint32_t srcFamily, uint32_t dstFamily, SyncPoint srcSync,
            uint32_t baseMip = 0, uint32_t mipCount = vk::RemainingMipLevels) const noexcept;

        void recordOwnershipAcquire(
            vk::CommandBuffer cmd, uint32_t srcFamily, uint32_t dstFamily, SyncPoint dstSync,
            uint32_t baseMip = 0, uint32_t mipCount = vk::RemainingMipLevels) const noexcept;

        void recordOwnershipRelease(
            vk::CommandBuffer cmd, uint32_t srcFamily, uint32_t dstFamily, SyncPoint srcSync,
            vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
            uint32_t baseMip = 0, uint32_t mipCount = vk::RemainingMipLevels) const noexcept;

        void recordOwnershipAcquire(
            vk::CommandBuffer cmd, uint32_t srcFamily, uint32_t dstFamily, SyncPoint dstSync,
            vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
            uint32_t baseMip = 0, uint32_t mipCount = vk::RemainingMipLevels) const noexcept;

        [[nodiscard]] virtual SyncPoint getLayoutTransitionSrcSync(vk::ImageLayout oldLayout) const;
        [[nodiscard]] virtual SyncPoint getLayoutTransitionDstSync(vk::ImageLayout newLayout) const;

        virtual ~Image() = default;

    protected:
        [[nodiscard]] virtual vk::ImageAspectFlags getAspectMask() const noexcept;

        vk::Format _format{};

    public:
        template<ImageImpl I>
        class Builder final {
        public:
            Builder& usage(vk::ImageUsageFlags flags) noexcept;

            Builder& extent(uint32_t width, uint32_t height, uint32_t depth = 1) noexcept;
            Builder& extent(vk::Extent2D dims) noexcept;
            Builder& extent(vk::Extent3D dims) noexcept;

            Builder& format(vk::Format format) noexcept;
            Builder& tiling(vk::ImageTiling tiling) noexcept;

            Builder& initialLayout(vk::ImageLayout layout) noexcept;
            Builder& mipLevelCount(uint32_t levels) noexcept;

            Builder& sampleCount(vk::SampleCountFlagBits samples) noexcept;

            [[nodiscard]] I build(VmaAllocator allocator, uint32_t* mipLevelCount = nullptr) const;

        private:
            vk::ImageUsageFlags _usage{};
            vk::Extent3D _extent{};
            vk::Format _format{ vk::Format::eUndefined };
            vk::ImageAspectFlags _aspects{ vk::ImageAspectFlagBits::eColor };
            vk::ImageTiling _tiling{ vk::ImageTiling::eOptimal };
            vk::ImageLayout _layout{ vk::ImageLayout::eUndefined };
            uint32_t _mipLevelCount{ 1 };
            vk::SampleCountFlagBits _sampleCount{ vk::SampleCountFlagBits::e1 };
        };
    };

    struct SwapImage {
        vk::Image image{};
        uint32_t index{};

        operator vk::Image() const noexcept { return image; }
        operator bool() const noexcept { return static_cast<bool>(image); }

        void recordLayoutTransition(vk::CommandBuffer cmd, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) const;
    };
} // namespace tpd

inline tpd::Image::Image(const vk::Format format, const vk::Image image, VmaAllocation allocation) noexcept
    : OpaqueResource{ image, allocation }, _format{ format } {}

inline vk::ImageAspectFlags tpd::Image::getAspectMask() const noexcept {
    return vk::ImageAspectFlagBits::eColor;
}

template<tpd::ImageImpl I>
tpd::Image::Builder<I>& tpd::Image::Builder<I>::usage(const vk::ImageUsageFlags flags) noexcept {
    _usage = flags;
    return *this;
}

template<tpd::ImageImpl I>
tpd::Image::Builder<I>& tpd::Image::Builder<I>::extent(const uint32_t width, const uint32_t height, const uint32_t depth) noexcept {
    return extent(vk::Extent3D{ width, height, depth });
}

template<tpd::ImageImpl I>
tpd::Image::Builder<I>& tpd::Image::Builder<I>::extent(const vk::Extent2D dims) noexcept {
    return extent(vk::Extent3D{ dims.width, dims.height, 1 });
}

template<tpd::ImageImpl I>
tpd::Image::Builder<I>& tpd::Image::Builder<I>::extent(const vk::Extent3D dims) noexcept {
    _extent = dims;
    return *this;
}

template<tpd::ImageImpl I>
tpd::Image::Builder<I>& tpd::Image::Builder<I>::format(const vk::Format format) noexcept {
    _format = format;
    return *this;
}

template<tpd::ImageImpl I>
tpd::Image::Builder<I>& tpd::Image::Builder<I>::tiling(const vk::ImageTiling tiling) noexcept {
    _tiling = tiling;
    return *this;
}

template<tpd::ImageImpl I>
tpd::Image::Builder<I>& tpd::Image::Builder<I>::initialLayout(const vk::ImageLayout layout) noexcept {
    _layout = layout;
    return *this;
}

template<tpd::ImageImpl I>
tpd::Image::Builder<I>& tpd::Image::Builder<I>::mipLevelCount(const uint32_t levels) noexcept {
    _mipLevelCount = levels;
    return *this;
}

template<tpd::ImageImpl I>
tpd::Image::Builder<I>& tpd::Image::Builder<I>::sampleCount(const vk::SampleCountFlagBits samples) noexcept {
    _sampleCount = samples;
    return *this;
}

template<tpd::ImageImpl I>
I tpd::Image::Builder<I>::build(VmaAllocator allocator, uint32_t* mipLevelCount) const {
    if (_extent.width == 0 || _extent.height == 0 || _extent.depth == 0) [[unlikely]] {
        throw std::runtime_error(
            "Image::Builder - Image is being built with 0 dimensions: "
            "did you forget to call Image::Builder::extent()?");
    }

    if (_format == vk::Format::eUndefined) [[unlikely]] {
        throw std::runtime_error(
            "Image::Builder - Could NOT create an image with undefined format: "
            "did you forget to call Image::Builder::format()?");
    }

    if (_layout != vk::ImageLayout::eUndefined && _layout != vk::ImageLayout::ePreinitialized) [[unlikely]] {
        throw std::runtime_error(
            "Target::Builder - Image cannot be initialized with a layout other than eUndefined or ePreinitialized");
    }

    const auto mipCount = _mipLevelCount > 0
        ? _mipLevelCount
        : static_cast<uint32_t>(std::floor(std::log2(std::max(_extent.width, _extent.height)))) + 1;

    if (mipLevelCount) {
        *mipLevelCount = mipCount;
    }

    auto imageCreateInfo = vk::ImageCreateInfo{};
    imageCreateInfo.usage = _usage;
    imageCreateInfo.extent = _extent;
    imageCreateInfo.format = _format;
    imageCreateInfo.tiling = _tiling;
    imageCreateInfo.initialLayout = _layout;
    imageCreateInfo.mipLevels = mipCount;
    imageCreateInfo.imageType = _extent.depth > 1 ? vk::ImageType::e3D : _extent.height > 1 ? vk::ImageType::e2D : vk::ImageType::e1D;
    imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = _sampleCount;

    auto allocation = VmaAllocation{};
    const auto image = vma::allocateDeviceImage(allocator, imageCreateInfo, &allocation);
    return I{ _format, image, allocation };
}
