#pragma once

#include <vulkan/vulkan.hpp>

namespace tpd {
    class SamplerBuilder {
    public:
        SamplerBuilder& filter(vk::Filter magnifiedFilter, vk::Filter minifiedFilter);
        SamplerBuilder& addressMode(vk::SamplerAddressMode modeU, vk::SamplerAddressMode modeV, vk::SamplerAddressMode modeW);

        SamplerBuilder& anisotropyEnabled(bool enabled);
        SamplerBuilder& maxAnisotropy(float value);

        SamplerBuilder& borderColor(vk::BorderColor color);

        [[nodiscard]] vk::Sampler build(vk::Device device) const;

    private:
        vk::Filter _magFilter{ vk::Filter::eLinear };
        vk::Filter _minFilter{ vk::Filter::eLinear };

        vk::SamplerAddressMode _addressModeU{ vk::SamplerAddressMode::eRepeat };
        vk::SamplerAddressMode _addressModeV{ vk::SamplerAddressMode::eRepeat };
        vk::SamplerAddressMode _addressModeW{ vk::SamplerAddressMode::eRepeat };

        vk::Bool32 _anisotropyEnabled{ vk::False };
        float _maxAnisotropy{ 1.0f };

        vk::BorderColor _borderColor{ vk::BorderColor::eIntOpaqueBlack };

        vk::SamplerMipmapMode _mipmapMode{ vk::SamplerMipmapMode::eLinear };
        float _mipLodBias{ 0.0f };
        float _minLod{ 0.0f };
        float _maxLod{ 0.0f };
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::SamplerBuilder& tpd::SamplerBuilder::filter(const vk::Filter magnifiedFilter, const vk::Filter minifiedFilter) {
    _magFilter = magnifiedFilter;
    _minFilter = minifiedFilter;
    return *this;
}

inline tpd::SamplerBuilder& tpd::SamplerBuilder::addressMode(
    const vk::SamplerAddressMode modeU,
    const vk::SamplerAddressMode modeV,
    const vk::SamplerAddressMode modeW)
{
    _addressModeU = modeU;
    _addressModeV = modeV;
    _addressModeW = modeW;
    return *this;
}

inline tpd::SamplerBuilder& tpd::SamplerBuilder::anisotropyEnabled(const bool enabled) {
    _anisotropyEnabled = enabled;
    return *this;
}

inline tpd::SamplerBuilder& tpd::SamplerBuilder::maxAnisotropy(const float value) {
    _maxAnisotropy = value;
    return *this;
}

inline tpd::SamplerBuilder& tpd::SamplerBuilder::borderColor(const vk::BorderColor color) {
    _borderColor = color;
    return *this;
}
