#include "torpedo/foundation/Image.h"
#include "torpedo/foundation/ImageUtils.h"

vk::ImageView tpd::Image::createImageView(
    const vk::ImageViewType type,
    const vk::Device device,
    const vk::ImageViewCreateFlags flags) const
{
    auto imageViewInfo = vk::ImageViewCreateInfo{};
    imageViewInfo.flags = flags;
    imageViewInfo.image = _resource;
    imageViewInfo.format = _format;
    imageViewInfo.viewType = type;
    imageViewInfo.subresourceRange = { getAspectMask(), 0, vk::RemainingMipLevels, 0, vk::RemainingArrayLayers };
    imageViewInfo.components.r = vk::ComponentSwizzle::eIdentity;
    imageViewInfo.components.b = vk::ComponentSwizzle::eIdentity;
    imageViewInfo.components.g = vk::ComponentSwizzle::eIdentity;
    imageViewInfo.components.a = vk::ComponentSwizzle::eIdentity;

    return device.createImageView(imageViewInfo);
}

void tpd::Image::recordLayoutTransition(
    const vk::CommandBuffer cmd,
    const vk::ImageLayout oldLayout,
    const vk::ImageLayout newLayout,
    const uint32_t baseMip,
    const uint32_t mipCount) const noexcept
{
    if (oldLayout == newLayout) [[unlikely]] {
        return;
    }

    const auto [srcStage, srcAccess] = getLayoutTransitionSrcSync(oldLayout);
    const auto [dstStage, dstAccess] = getLayoutTransitionDstSync(newLayout);

    auto barrier = vk::ImageMemoryBarrier2{};
    barrier.image = _resource;
    barrier.subresourceRange = { getAspectMask(), baseMip, mipCount, 0, 1 };
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcStageMask = srcStage;
    barrier.dstStageMask = dstStage;
    barrier.srcAccessMask = srcAccess;
    barrier.dstAccessMask = dstAccess;
    barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;

    const auto dependency = vk::DependencyInfo{}
    .setImageMemoryBarrierCount(1)
    .setPImageMemoryBarriers(&barrier);
    cmd.pipelineBarrier2(dependency);
}

void tpd::Image::recordOwnershipRelease(
    const vk::CommandBuffer cmd,
    const uint32_t srcFamily,
    const uint32_t dstFamily,
    const SyncPoint srcSync,
    const uint32_t baseMip,
    const uint32_t mipCount) const noexcept
{
    auto barrier = vk::ImageMemoryBarrier2{};
    barrier.image = _resource;
    barrier.subresourceRange = { getAspectMask(), baseMip, mipCount, 0, vk::RemainingArrayLayers };
    barrier.srcQueueFamilyIndex = srcFamily;
    barrier.dstQueueFamilyIndex = dstFamily;
    barrier.srcStageMask  = srcSync.stage;
    barrier.srcAccessMask = srcSync.access;

    // TODO: consider VK_DEPENDENCY_QUEUE_FAMILY_OWNERSHIP_TRANSFER_USE_ALL_STAGES_BIT_KHR to avoid full pipeline stalls
    const auto dependency = vk::DependencyInfo{}.setImageMemoryBarriers(barrier);
    cmd.pipelineBarrier2(dependency);
}

void tpd::Image::recordOwnershipAcquire(
    const vk::CommandBuffer cmd,
    const uint32_t srcFamily,
    const uint32_t dstFamily,
    const SyncPoint dstSync,
    const uint32_t baseMip,
    const uint32_t mipCount) const noexcept
{
    auto barrier = vk::ImageMemoryBarrier2{};
    barrier.image = _resource;
    barrier.subresourceRange = { getAspectMask(), baseMip, mipCount, 0, vk::RemainingArrayLayers };
    barrier.srcQueueFamilyIndex = srcFamily;
    barrier.dstQueueFamilyIndex = dstFamily;
    barrier.dstStageMask  = dstSync.stage;
    barrier.dstAccessMask = dstSync.access;

    // TODO: consider VK_DEPENDENCY_QUEUE_FAMILY_OWNERSHIP_TRANSFER_USE_ALL_STAGES_BIT_KHR to avoid full pipeline stalls
    const auto dependency = vk::DependencyInfo{}.setImageMemoryBarriers(barrier);
    cmd.pipelineBarrier2(dependency);
}

void tpd::Image::recordOwnershipRelease(
    const vk::CommandBuffer cmd,
    const uint32_t srcFamily,
    const uint32_t dstFamily,
    const SyncPoint srcSync,
    const vk::ImageLayout oldLayout,
    const vk::ImageLayout newLayout,
    const uint32_t baseMip,
    const uint32_t mipCount) const noexcept
{
    auto barrier = vk::ImageMemoryBarrier2{};
    barrier.image = _resource;
    barrier.subresourceRange = { getAspectMask(), baseMip, mipCount, 0, vk::RemainingArrayLayers };
    barrier.srcQueueFamilyIndex = srcFamily;
    barrier.dstQueueFamilyIndex = dstFamily;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcStageMask  = srcSync.stage;
    barrier.srcAccessMask = srcSync.access;

    // TODO: consider VK_DEPENDENCY_QUEUE_FAMILY_OWNERSHIP_TRANSFER_USE_ALL_STAGES_BIT_KHR to avoid full pipeline stalls
    const auto dependency = vk::DependencyInfo{}.setImageMemoryBarriers(barrier);
    cmd.pipelineBarrier2(dependency);
}

void tpd::Image::recordOwnershipAcquire(
    const vk::CommandBuffer cmd,
    const uint32_t srcFamily,
    const uint32_t dstFamily,
    const SyncPoint dstSync,
    const vk::ImageLayout oldLayout,
    const vk::ImageLayout newLayout,
    const uint32_t baseMip,
    const uint32_t mipCount) const noexcept
{
    auto barrier = vk::ImageMemoryBarrier2{};
    barrier.image = _resource;
    barrier.subresourceRange = { getAspectMask(), baseMip, mipCount, 0, vk::RemainingArrayLayers };
    barrier.srcQueueFamilyIndex = srcFamily;
    barrier.dstQueueFamilyIndex = dstFamily;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.dstStageMask  = dstSync.stage;
    barrier.dstAccessMask = dstSync.access;

    // TODO: consider VK_DEPENDENCY_QUEUE_FAMILY_OWNERSHIP_TRANSFER_USE_ALL_STAGES_BIT_KHR to avoid full pipeline stalls
    const auto dependency = vk::DependencyInfo{}.setImageMemoryBarriers(barrier);
    cmd.pipelineBarrier2(dependency);
}

void tpd::SwapImage::recordLayoutTransition(
    const vk::CommandBuffer cmd,
    const vk::ImageLayout oldLayout,
    const vk::ImageLayout newLayout) const
{
    if (oldLayout == newLayout) [[unlikely]] {
        return;
    }
    using PipelineStage = vk::PipelineStageFlagBits2;
    using AccessMask = vk::AccessFlagBits2;

    vk::PipelineStageFlags2 srcStage = PipelineStage::eTopOfPipe;
    vk::AccessFlags2 srcAccessMask = AccessMask::eNone;

    vk::PipelineStageFlags2 dstStage = PipelineStage::eBottomOfPipe;
    vk::AccessFlags2 dstAccessMask = AccessMask::eNone;

    using enum vk::ImageLayout;
    switch (oldLayout) {
    case eUndefined:
        break;

    case eTransferDstOptimal:
        srcStage = PipelineStage::eTransfer;
        srcAccessMask = AccessMask::eTransferWrite;
        break;

    [[unlikely]] default:
        throw std::invalid_argument(
            "SwapImage - Unsupported image layout transition with " + utils::toString(oldLayout) + " as src layout");
    }

    switch (newLayout) {
    case eTransferDstOptimal:
        dstStage = PipelineStage::eTransfer;
        dstAccessMask = AccessMask::eTransferWrite;
        break;

    case ePresentSrcKHR:
        // vkQueuePresentKHR performs automatic visibility operations
        break;

    [[unlikely]] default:
        throw std::invalid_argument(
            "SwapImage - Unsupported image layout transition with " + utils::toString(newLayout) + " as dst layout");
    }

    auto barrier = vk::ImageMemoryBarrier2{};
    barrier.image = image;
    barrier.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcStageMask = srcStage;
    barrier.dstStageMask = dstStage;
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;
    barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;

    const auto dependency = vk::DependencyInfo{}
        .setImageMemoryBarrierCount(1)
        .setPImageMemoryBarriers(&barrier);
    cmd.pipelineBarrier2(dependency);
}

tpd::SyncPoint tpd::Image::getLayoutTransitionSrcSync(const vk::ImageLayout oldLayout) const {
    using PipelineStage = vk::PipelineStageFlagBits2;
    using AccessMask    = vk::AccessFlagBits2;

    vk::PipelineStageFlags2 srcStage = PipelineStage::eTopOfPipe;
    vk::AccessFlags2 srcAccessMask   = AccessMask::eNone;

    constexpr auto depthStage   = PipelineStage::eEarlyFragmentTests | PipelineStage::eLateFragmentTests;
    constexpr auto sampledStage = PipelineStage::eVertexShader | PipelineStage::eFragmentShader | PipelineStage::eComputeShader;

    using enum vk::ImageLayout;
    switch (oldLayout) {
    case eUndefined:
    case ePresentSrcKHR:
        break; // top of pipe, no access

    case eGeneral:
        srcStage = PipelineStage::eAllCommands;
        srcAccessMask = AccessMask::eMemoryWrite;
        break;

    case eColorAttachmentOptimal:
        srcStage = PipelineStage::eColorAttachmentOutput;
        srcAccessMask = AccessMask::eColorAttachmentWrite;
        break;

    case eDepthStencilAttachmentOptimal:
        srcStage = depthStage;
        srcAccessMask = AccessMask::eDepthStencilAttachmentWrite;
        break;

    case eDepthStencilReadOnlyOptimal:
        srcStage = depthStage | sampledStage;
        break;

    case eShaderReadOnlyOptimal:
        srcStage = sampledStage;
        break;

    case eTransferSrcOptimal:
        srcStage = PipelineStage::eTransfer;
        break;

    case eTransferDstOptimal:
        srcStage = PipelineStage::eTransfer;
        srcAccessMask = AccessMask::eTransferWrite;
        break;

    case ePreinitialized:
        srcStage = PipelineStage::eHost;
        srcAccessMask = AccessMask::eHostWrite;
        break;

    [[unlikely]] default:
        throw std::invalid_argument("Image - Unsupported src point for layout: " + utils::toString(oldLayout));
    }

    return { srcStage, srcAccessMask };
}

tpd::SyncPoint tpd::Image::getLayoutTransitionDstSync(const vk::ImageLayout newLayout) const {
    using PipelineStage = vk::PipelineStageFlagBits2;
    using AccessMask    = vk::AccessFlagBits2;

    constexpr auto depthStage   = PipelineStage::eEarlyFragmentTests | PipelineStage::eLateFragmentTests;
    constexpr auto sampledStage = PipelineStage::eVertexShader | PipelineStage::eFragmentShader | PipelineStage::eComputeShader;

    vk::PipelineStageFlags2 dstStage = PipelineStage::eBottomOfPipe;
    vk::AccessFlags2 dstAccessMask = AccessMask::eNone;

    using enum vk::ImageLayout;
    switch (newLayout) {
    case eGeneral:
    case eFragmentDensityMapOptimalEXT:
        dstStage = PipelineStage::eAllCommands;
        dstAccessMask = AccessMask::eMemoryWrite | AccessMask::eMemoryRead;
        break;

    case eColorAttachmentOptimal:
        dstStage = PipelineStage::eColorAttachmentOutput;
        dstAccessMask = AccessMask::eColorAttachmentWrite | AccessMask::eColorAttachmentRead;
        break;

    case eDepthStencilAttachmentOptimal:
        dstStage = depthStage;
        dstAccessMask = AccessMask::eDepthStencilAttachmentWrite | AccessMask::eDepthStencilAttachmentRead;
        break;

    case eDepthStencilReadOnlyOptimal:
        dstStage = depthStage | sampledStage;
        dstAccessMask = AccessMask::eDepthStencilAttachmentRead | AccessMask::eShaderSampledRead | AccessMask::eInputAttachmentRead;
        break;

    case eShaderReadOnlyOptimal:
        dstStage = sampledStage;
        dstAccessMask = AccessMask::eShaderSampledRead | AccessMask::eInputAttachmentRead;
        break;

    case eTransferSrcOptimal:
        dstStage = PipelineStage::eTransfer;
        dstAccessMask = AccessMask::eTransferRead;
        break;

    case eTransferDstOptimal:
        dstStage = PipelineStage::eTransfer;
        dstAccessMask = AccessMask::eTransferWrite;
        break;

    case ePresentSrcKHR:
        // vkQueuePresentKHR performs automatic visibility operations
        break;

    [[unlikely]] default:
        throw std::invalid_argument("Image - Unsupported dst point for layout: " + utils::toString(newLayout));
    }

    return { dstStage, dstAccessMask };
}
