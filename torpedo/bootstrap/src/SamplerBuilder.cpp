#include "torpedo/bootstrap/SamplerBuilder.h"

vk::Sampler tpd::SamplerBuilder::build(const vk::Device device) const {
    auto samplerInfo = vk::SamplerCreateInfo{};
    samplerInfo.magFilter = _magFilter;  // for over-sampling
    samplerInfo.minFilter = _minFilter;  // for under-sampling
    samplerInfo.addressModeU = _addressModeU;
    samplerInfo.addressModeV = _addressModeV;
    samplerInfo.addressModeW = _addressModeW;
    // The maxAnisotropy field limits the number of texel samples that can be used to calculate the final color.
    // A lower value results in better performance, but lower quality results. A value of 1.0f disables this effect.
    samplerInfo.anisotropyEnable = _anisotropyEnabled;
    samplerInfo.maxAnisotropy = _maxAnisotropy;
    // Which color is returned when sampling beyond the image with clamp to border addressing mode
    samplerInfo.borderColor = _borderColor;
    // TODO: add support for mipmap
    samplerInfo.mipmapMode = _mipmapMode;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    // Values that are uncommon or will be supported in the future
    samplerInfo.unnormalizedCoordinates = vk::False;
    samplerInfo.compareEnable = vk::False;
    samplerInfo.compareOp = vk::CompareOp::eAlways;

    return device.createSampler(samplerInfo);
}
