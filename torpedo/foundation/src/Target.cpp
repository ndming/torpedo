#include "torpedo/foundation/Target.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include <cstdint>

tpd::Target tpd::Target::Builder::build(const DeviceAllocator& allocator) const {
    auto allocation = VmaAllocation{};
    const auto image = createImage(allocator, &allocation);
    return Target{ vk::Extent3D{ _w, _h, _d }, _aspects, _tiling, 
        image, _layout, _format, allocation, allocator, _data, _dataSize };
}

std::unique_ptr<tpd::Target, tpd::Deleter<tpd::Target>>
tpd::Target::Builder::build(const DeviceAllocator& allocator, std::pmr::memory_resource* pool) const {
    auto allocation = VmaAllocation{};
    const auto image = createImage(allocator, &allocation);
    return foundation::make_unique<Target>(pool, vk::Extent3D{ _w, _h, _d }, _aspects, _tiling, 
        image, _layout, _format, allocation, allocator, _data, _dataSize);
}

vk::Image tpd::Target::Builder::createImage(const DeviceAllocator& allocator, VmaAllocation* allocation) const {
    if (_w == 0 || _h == 0 || _d == 0) [[unlikely]] {
        throw std::runtime_error(
            "Target::Builder - Image is being built with 0 dimensions: "
            "did you forget to call Target::Builder::extent()?");
    }

    if (_format == vk::Format::eUndefined) [[unlikely]] {
        throw std::runtime_error(
            "Target::Builder - Image cannot be initialized with an undefined format (except in very rare cases "
            "which we don't support): did you forget to call Target::Builder::format()?");
    }

    if (_aspects == vk::ImageAspectFlagBits::eNone) [[unlikely]] {
        throw std::runtime_error(
            "Target::Builder - Image cannot be initialized with no aspects: "
            "did you forget to call Target::Builder::aspect()?");
    }

    if (_layout != vk::ImageLayout::eUndefined && _layout != vk::ImageLayout::ePreinitialized) [[unlikely]] {
        throw std::runtime_error(
            "Target::Builder - Image cannot be initialized with a layout other than eUndefined or ePreinitialized");
    }

    const auto imageCreateInfo = vk::ImageCreateInfo{}
        .setImageType(_d > 1 ? vk::ImageType::e3D : _h > 1 ? vk::ImageType::e2D : vk::ImageType::e1D)
        .setExtent(vk::Extent3D{_w, _h, _d})
        .setUsage(_usage)
        .setTiling(_tiling)
        .setSharingMode(vk::SharingMode::eExclusive)
        .setInitialLayout(_layout)
        .setSamples(vk::SampleCountFlagBits::e1)  // only applicable to attachments
        .setMipLevels(1)     // multi-level targets are not supported at the moment
        .setFormat(_format)  // must not be undefined, except when working with Android builds
        .setArrayLayers(1);  // multi-view targets are not supported at the moment
    return allocator.allocateDeviceImage(imageCreateInfo, allocation);
}

void tpd::Target::recordOwnershipRelease(
    const vk::CommandBuffer cmd, 
    const uint32_t srcFamilyIndex, 
    const uint32_t dstFamilyIndex) const noexcept 
{
    auto barrier = vk::ImageMemoryBarrier2{};
    barrier.image = _image;
    barrier.subresourceRange = { _aspects, 0, vk::RemainingMipLevels, 0, vk::RemainingArrayLayers };
    barrier.srcQueueFamilyIndex = srcFamilyIndex;
    barrier.dstQueueFamilyIndex = dstFamilyIndex;
    // The implementation assumming we're writing to the image in a compute shader
    // and then copying it to the swap chain image for presentation
    barrier.oldLayout = vk::ImageLayout::eGeneral;
    barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
    barrier.srcStageMask  = vk::PipelineStageFlagBits2::eComputeShader;  // release after compute write
    barrier.srcAccessMask = vk::AccessFlagBits2::eShaderStorageWrite;    // ensure image write persists

    // TODO: consider VK_DEPENDENCY_QUEUE_FAMILY_OWNERSHIP_TRANSFER_USE_ALL_STAGES_BIT_KHR to avoid full pipeline stalls
    const auto dependency = vk::DependencyInfo{}.setImageMemoryBarriers(barrier);
    cmd.pipelineBarrier2(dependency);
}

void tpd::Target::recordOwnershipAcquire(
    const vk::CommandBuffer cmd,
    const uint32_t srcFamilyIndex,
    const uint32_t dstFamilyIndex) const noexcept
{
    auto barrier = vk::ImageMemoryBarrier2{};
    barrier.image = _image;
    barrier.subresourceRange = { _aspects, 0, vk::RemainingMipLevels, 0, vk::RemainingArrayLayers };
    barrier.srcQueueFamilyIndex = srcFamilyIndex;
    barrier.dstQueueFamilyIndex = dstFamilyIndex;
    // The implementation assumming we're writing to the image in a compute shader
    // and then copying it to the swap chain image for presentation
    barrier.oldLayout = vk::ImageLayout::eGeneral;
    barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
    barrier.dstStageMask  = vk::PipelineStageFlagBits2::eTransfer;  // copy happens at transfer
    barrier.dstAccessMask = vk::AccessFlagBits2::eTransferRead;     // ensure memory is visible for copy

    // TODO: consider VK_DEPENDENCY_QUEUE_FAMILY_OWNERSHIP_TRANSFER_USE_ALL_STAGES_BIT_KHR to avoid full pipeline stalls
    const auto dependency = vk::DependencyInfo{}.setImageMemoryBarriers(barrier);
    cmd.pipelineBarrier2(dependency);
}
