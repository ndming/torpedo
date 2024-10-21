#pragma once

#include <torpedo/foundation/Image.h>

namespace tpd {
    class Texture final {
        static vk::Sampler _defaultSampler;
        static const ResourceAllocator* _allocator;

        friend class Renderer;

    public:
        class Builder {
        public:
            Builder& size(uint32_t width, uint32_t height);

            Builder& withSeparateSampler();
            Builder& filter(vk::Filter magnifiedFilter, vk::Filter minifiedFilter);
            Builder& maxAnisotropy(float value);

            [[nodiscard]] std::unique_ptr<Texture> build(const Renderer& renderer) const;

        private:
            uint32_t _width{ 0 };
            uint32_t _height{ 0 };

            bool _separateSampler{ false };
            vk::Filter _magFilter{ vk::Filter::eLinear };
            vk::Filter _minFilter{ vk::Filter::eLinear };
            float _maxAnisotropy{ 1.0f };
        };

        Texture(vk::Device device,std::unique_ptr<Image> image, vk::Sampler sampler = {});

        void setImageData(const void* data, uint32_t width, uint32_t height, const Renderer& renderer) const;

        [[nodiscard]] const std::unique_ptr<Image>& getImage() const;
        [[nodiscard]] vk::Sampler getSampler() const;

        static vk::Sampler getDefaultSampler();

        void dispose() noexcept;

        ~Texture();

    private:
        vk::Device _device;
        std::unique_ptr<Image> _image;
        vk::Sampler _sampler;
    };
}

inline vk::Sampler tpd::Texture::_defaultSampler = {};
inline const tpd::ResourceAllocator* tpd::Texture::_allocator = nullptr;

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::Texture::Builder& tpd::Texture::Builder::size(const uint32_t width, const uint32_t height) {
    _width = width;
    _height = height;
    return *this;
}

inline tpd::Texture::Builder& tpd::Texture::Builder::withSeparateSampler() {
    _separateSampler = true;
    return *this;
}

inline tpd::Texture::Builder &tpd::Texture::Builder::filter(const vk::Filter magnifiedFilter, const vk::Filter minifiedFilter) {
    _magFilter = magnifiedFilter;
    _minFilter = minifiedFilter;
    return *this;
}

inline tpd::Texture::Builder &tpd::Texture::Builder::maxAnisotropy(const float value) {
    _maxAnisotropy = value;
    return *this;
}

inline tpd::Texture::Texture(const vk::Device device, std::unique_ptr<Image> image, const vk::Sampler sampler)
    : _device{ device }, _image{ std::move(image) }, _sampler{ sampler } {
}

inline const std::unique_ptr<tpd::Image>& tpd::Texture::getImage() const {
    return _image;
}

inline vk::Sampler tpd::Texture::getSampler() const {
    return _sampler ? _sampler : _defaultSampler;
}

inline vk::Sampler tpd::Texture::getDefaultSampler() {
    return _defaultSampler;
}
