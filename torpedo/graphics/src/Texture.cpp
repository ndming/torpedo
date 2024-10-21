#include "torpedo/graphics/Texture.h"
#include "torpedo/graphics/Renderer.h"

#include <torpedo/bootstrap/SamplerBuilder.h>

#include <plog/Log.h>

std::unique_ptr<tpd::Texture> tpd::Texture::Builder::build(const Renderer& renderer) const {
    const auto anisotropyLimit = renderer.getMaxSamplerAnisotropy();
    if (_separateSampler && _maxAnisotropy > anisotropyLimit) {
        PLOGW << "Texture::Builder - Exceed maximum anisotropic support, clamping to the limit";
    }

    const auto device = renderer.getVulkanDevice();

    const auto sampler = _separateSampler ? SamplerBuilder()
        .filter(_magFilter, _minFilter)
        .anisotropyEnabled(true)
        .maxAnisotropy(_maxAnisotropy > anisotropyLimit ? anisotropyLimit : _maxAnisotropy)
        .build(device) : vk::Sampler{};

    auto image = Image::Builder()
        .dimensions(_width, _height, 1)
        .type(vk::ImageType::e2D)
        .format(vk::Format::eR8G8B8A8Srgb)
        .usage(vk::ImageUsageFlagBits::eSampled)
        .imageViewType(vk::ImageViewType::e2D)
        .imageViewAspects(vk::ImageAspectFlagBits::eColor)
        .build(device, renderer.getResourceAllocator(), ResourceType::Dedicated);

    renderer.transitionImageLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal, image->getImage());

    return std::make_unique<Texture>(device, std::move(image), sampler);
}

void tpd::Texture::setImageData(const void* data, const uint32_t width, const uint32_t height, const Renderer& renderer) const {
    const auto layoutTransition = [&renderer](const vk::ImageLayout oldLayout, const vk::ImageLayout newLayout, const vk::Image image) {
        renderer.transitionImageLayout(oldLayout, newLayout, image);
    };
    const auto bufferToImageCopy = [&renderer, &width, &height](const vk::Buffer buffer, const vk::Image image) {
        auto copyInfo = vk::BufferImageCopy{};
        copyInfo.bufferOffset = 0;
        copyInfo.bufferRowLength = 0;
        copyInfo.bufferImageHeight = 0;
        copyInfo.imageOffset = vk::Offset3D{ 0, 0, 0 };
        copyInfo.imageExtent = vk::Extent3D{ width, height, 1 };
        copyInfo.imageSubresource = { vk::ImageAspectFlagBits::eColor, /* mip level */ 0, 0, 1 };

        renderer.copyBufferToImage(buffer, image, copyInfo);
    };

    _image->transferImageData(
        data, width * height * 4, vk::ImageLayout::eShaderReadOnlyOptimal,
        renderer.getResourceAllocator(), layoutTransition, bufferToImageCopy);
}

void tpd::Texture::dispose() noexcept {
    _device.destroySampler(_sampler);
    if (_allocator && _image) {
        _image->dispose(_device, *_allocator);
        _image.reset();
    }
}

tpd::Texture::~Texture() {
    dispose();
}
