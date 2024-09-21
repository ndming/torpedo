#pragma once

#include "torpedo/foundation/ResourceAllocator.h"

#include <vulkan/vulkan.hpp>

#include <functional>

namespace tpd {
    class Image final {
    public:
        class Builder {
        public:
            Builder& dimensions(uint32_t width, uint32_t height, uint32_t depth = 1);
            Builder& type(vk::ImageType type);
            Builder& mipLevels(uint32_t levels);
            Builder& format(vk::Format format);
            Builder& usage(vk::ImageUsageFlags usage);

            Builder& imageViewType(vk::ImageViewType type);
            Builder& imageViewAspects(vk::ImageAspectFlags viewAspects);
            Builder& imageViewBaseMipLevel(uint32_t baseLevel);

            [[nodiscard]] std::unique_ptr<Image> buildDedicated(
                const std::unique_ptr<ResourceAllocator>& allocator,
                vk::Device device,
                vk::ImageCreateFlags flags = {}) const;

        private:
            [[nodiscard]] vk::ImageCreateInfo populateImageCreateInfo(
                vk::ImageUsageFlags internalUsage,
                vk::ImageTiling tiling,
                vk::ImageCreateFlags flags) const;

            uint32_t _width{ 0 };
            uint32_t _height{ 0 };
            uint32_t _depth{ 0 };
            vk::ImageType _type{ vk::ImageType::e2D };
            uint32_t _mipLevels{ 1 };
            vk::Format _format{ vk::Format::eUndefined };
            vk::ImageUsageFlags _usage{};

            vk::ImageViewType _viewType{ vk::ImageViewType::e2D };
            vk::ImageAspectFlags _viewAspects{};
            uint32_t _viewBaseMipLevel{ 0 };
        };

        Image(vk::Image image, vk::ImageView imageView, VmaAllocation allocation);

        Image(const Image&) = delete;
        Image& operator=(const Image&) = delete;

        void transferImageData(
            const void* data,
            std::uint32_t dataByteSize,
            vk::ImageLayout finalLayout,
            const std::unique_ptr<ResourceAllocator>& allocator,
            const std::function<void(vk::ImageLayout, vk::ImageLayout, vk::Image)>& onLayoutTransition,
            const std::function<void(vk::Buffer, vk::Image)>& onBufferToImageCopy) const;

        [[nodiscard]] vk::Image getImage() const;
        [[nodiscard]] vk::ImageView getImageView() const;

        void destroy(vk::Device device, const std::unique_ptr<ResourceAllocator>& allocator) const noexcept;

    private:
        vk::Image _image;
        vk::ImageView _imageView;
        VmaAllocation _allocation;
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::Image::Builder&tpd::Image::Builder::dimensions(const uint32_t width, const uint32_t height, const uint32_t depth) {
    _width = width;
    _height = height;
    _depth = depth;
    return *this;
}

inline tpd::Image::Builder& tpd::Image::Builder::type(const vk::ImageType type) {
    _type = type;
    return *this;
}

inline tpd::Image::Builder& tpd::Image::Builder::mipLevels(const uint32_t levels) {
    _mipLevels = levels;
    return *this;
}

inline tpd::Image::Builder&tpd::Image::Builder::format(const vk::Format format) {
    _format = format;
    return *this;
}

inline tpd::Image::Builder& tpd::Image::Builder::usage(const vk::ImageUsageFlags usage) {
    _usage = usage;
    return *this;
}

inline tpd::Image::Builder& tpd::Image::Builder::imageViewType(const vk::ImageViewType type) {
    _viewType = type;
    return *this;
}

inline tpd::Image::Builder& tpd::Image::Builder::imageViewAspects(const vk::ImageAspectFlags viewAspects) {
    _viewAspects = viewAspects;
    return *this;
}

inline tpd::Image::Builder& tpd::Image::Builder::imageViewBaseMipLevel(const uint32_t baseLevel) {
    _viewBaseMipLevel = baseLevel;
    return *this;
}

inline tpd::Image::Image(const vk::Image image, const vk::ImageView imageView, VmaAllocation allocation)
: _image{ image }, _imageView{ imageView }, _allocation{ allocation } {
}

inline vk::Image tpd::Image::getImage() const {
    return _image;
}

inline vk::ImageView tpd::Image::getImageView() const {
    return _imageView;
}
