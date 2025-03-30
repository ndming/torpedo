#pragma once

#include "torpedo/foundation/DeviceAllocator.h"
#include "torpedo/foundation/SyncResource.h"

namespace tpd {
    class Image : public Destroyable {
    public:
        Image(
            vk::Image image, vk::ImageLayout layout, vk::Format format,
            VmaAllocation allocation, const DeviceAllocator& allocator);

        Image(Image&&) noexcept = default;
        Image& operator=(Image&&) noexcept = default;

        void recordImageTransition(vk::CommandBuffer cmd, vk::ImageLayout newLayout);

        [[nodiscard]] vk::ImageView createImageView(vk::ImageViewType type, vk::Device device, vk::ImageViewCreateFlags flags = {}) const;

        void destroy() noexcept override;
        ~Image() noexcept override;

    protected:
        [[nodiscard]] virtual vk::ImageAspectFlags getAspectMask() const noexcept = 0;
        [[nodiscard]] virtual uint32_t getMipLevelCount() const noexcept;

        [[nodiscard]] virtual SyncPoint getTransitionSrcPoint(vk::ImageLayout oldLayout) const;
        [[nodiscard]] virtual SyncPoint getTransitionDstPoint(vk::ImageLayout newLayout) const;

        vk::Image _image;
        vk::ImageLayout _layout;
        vk::Format _format;
    };
}  // namespace tpd

inline tpd::Image::Image(
    const vk::Image image, const vk::ImageLayout layout, const vk::Format format,
    VmaAllocation allocation, const DeviceAllocator& allocator)
    : Destroyable{ allocation, allocator }, _image{ image }, _layout{ layout }, _format{ format } {
    if (!image || !allocation) [[unlikely]] {
        throw std::invalid_argument("Image - vk::Image is in invalid state: consider using a builder");
    }
}

inline uint32_t tpd::Image::getMipLevelCount() const noexcept {
    return 1;
}

inline void tpd::Image::destroy() noexcept {
    if (_allocation) {
        _allocator.deallocateImage(_image, _allocation);
    }
    Destroyable::destroy();
}

inline tpd::Image::~Image() noexcept {
    Image::destroy();
}
