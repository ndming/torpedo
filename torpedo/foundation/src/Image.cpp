#include "torpedo/foundation/Image.h"
#include "torpedo/foundation/ImageUtils.h"

vk::ImageView tpd::Image::createImageView(
    const vk::ImageViewType type,
    const vk::Device device,
    const vk::ImageViewCreateFlags flags) const
{
    auto imageViewInfo = vk::ImageViewCreateInfo{};
    imageViewInfo.flags = flags;
    imageViewInfo.image = _image;
    imageViewInfo.format = _format;
    imageViewInfo.viewType = type;
    imageViewInfo.subresourceRange = { getAspectMask(), 0, getMipLevelCount(), 0, vk::RemainingArrayLayers };
    imageViewInfo.components.r = vk::ComponentSwizzle::eIdentity;
    imageViewInfo.components.b = vk::ComponentSwizzle::eIdentity;
    imageViewInfo.components.g = vk::ComponentSwizzle::eIdentity;
    imageViewInfo.components.a = vk::ComponentSwizzle::eIdentity;

    return device.createImageView(imageViewInfo);
}

void tpd::Image::recordImageTransition(const vk::CommandBuffer cmd, const vk::ImageLayout newLayout) {
    const auto oldLayout = _layout;
    if (oldLayout == newLayout) {
        return;
    }

    const auto [srcStage, srcAccess] = getTransitionSrcPoint(oldLayout);
    const auto [dstStage, dstAccess] = getTransitionDstPoint(newLayout);

    auto barrier = vk::ImageMemoryBarrier2{};
    barrier.image = _image;
    barrier.subresourceRange = { getAspectMask(), 0, getMipLevelCount(), 0, 1 };
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
    _layout = newLayout;
}

std::pair<vk::PipelineStageFlags2, vk::AccessFlags2> tpd::Image::getTransitionSrcPoint(const vk::ImageLayout oldLayout) const {
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
        break;  // top of pipe, no access

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
        throw std::invalid_argument("Image - Unsupported src point for layout: " + foundation::toString(oldLayout));
    }

    return std::make_pair(srcStage, srcAccessMask);
}

std::pair<vk::PipelineStageFlags2, vk::AccessFlags2> tpd::Image::getTransitionDstPoint(const vk::ImageLayout newLayout) const {
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
        throw std::invalid_argument("Image - Unsupported dst point for layout: " + foundation::toString(newLayout));
    }

    return std::make_pair(dstStage, dstAccessMask);
}
