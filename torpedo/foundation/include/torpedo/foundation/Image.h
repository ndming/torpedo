#pragma once

#include "torpedo/foundation/DeviceAllocator.h"

namespace tpd {
    class Image : public Destroyable {
    public:
        Image(
            vk::Image image, vk::ImageLayout layout, vk::Format format,
            VmaAllocation allocation, const DeviceAllocator& allocator);

        void recordImageTransition(vk::CommandBuffer cmd, vk::ImageLayout newLayout);

        [[nodiscard]] vk::Image getVulkanImage() const noexcept;

        void destroy() noexcept override;
        ~Image() noexcept override;

    protected:
        [[nodiscard]] virtual vk::ImageAspectFlags getAspectMask() const noexcept = 0;
        [[nodiscard]] virtual uint32_t getMipLevelsCount() const noexcept;

        [[nodiscard]] virtual std::pair<vk::PipelineStageFlags2, vk::AccessFlags2> getSrcSync(vk::ImageLayout layout) const;
        [[nodiscard]] virtual std::pair<vk::PipelineStageFlags2, vk::AccessFlags2> getDstSync(vk::ImageLayout layout) const;

        vk::Image _image;
        vk::ImageLayout _layout;
        vk::Format _format;
    };
}

// =========================== //
// INLINE FUNCTION DEFINITIONS //
// =========================== //

inline tpd::Image::Image(
    const vk::Image image, const vk::ImageLayout layout, const vk::Format format,
    VmaAllocation allocation, const DeviceAllocator& allocator)
    : Destroyable{ allocation, allocator }, _image{ image }, _layout{ layout }, _format{ format } {
    if (!image || !allocation) [[unlikely]] {
        throw std::invalid_argument("Image - vk::Image is in invalid state: consider using a builder");
    }
}

inline vk::Image tpd::Image::getVulkanImage() const noexcept {
    return _image;
}

inline uint32_t tpd::Image::getMipLevelsCount() const noexcept {
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
