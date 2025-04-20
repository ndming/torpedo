#include "torpedo/foundation/ImageUtils.h"

std::string tpd::utils::toString(const vk::ImageLayout layout) {
    using enum vk::ImageLayout;
    switch (layout) {
        case eUndefined:                               return "Undefined";
        case eGeneral:                                 return "General";
        case eColorAttachmentOptimal:                  return "ColorAttachmentOptimal";
        case eDepthStencilAttachmentOptimal:           return "DepthStencilAttachmentOptimal";
        case eDepthStencilReadOnlyOptimal:             return "DepthStencilReadOnlyOptimal";
        case eShaderReadOnlyOptimal:                   return "ShaderReadOnlyOptimal";
        case eTransferSrcOptimal:                      return "TransferSrcOptimal";
        case eTransferDstOptimal:                      return "TransferDstOptimal";
        case ePreinitialized:                          return "Preinitialized";
        case eDepthReadOnlyStencilAttachmentOptimal:   return "DepthReadOnlyStencilAttachmentOptimal";
        case eDepthAttachmentStencilReadOnlyOptimal:   return "DepthAttachmentStencilReadOnlyOptimal";
        case eDepthAttachmentOptimal:                  return "DepthAttachmentOptimal";
        case eDepthReadOnlyOptimal:                    return "DepthReadOnlyOptimal";
        case eStencilAttachmentOptimal:                return "StencilAttachmentOptimal";
        case eStencilReadOnlyOptimal:                  return "StencilReadOnlyOptimal";
        case eReadOnlyOptimal:                         return "ReadOnlyOptimal";
        case eAttachmentOptimal:                       return "AttachmentOptimal";
        case eRenderingLocalRead:                      return "RenderingLocalRead";
        case ePresentSrcKHR:                           return "PresentSrcKHR";
        case eVideoDecodeDstKHR:                       return "VideoDecodeDstKHR";
        case eVideoDecodeSrcKHR:                       return "VideoDecodeSrcKHR";
        case eVideoDecodeDpbKHR:                       return "VideoDecodeDpbKHR";
        case eSharedPresentKHR:                        return "SharedPresentKHR";
        case eFragmentDensityMapOptimalEXT:            return "FragmentDensityMapOptimalEXT";
        case eFragmentShadingRateAttachmentOptimalKHR: return "FragmentShadingRateAttachmentOptimalKHR";
        case eVideoEncodeDstKHR:                       return "VideoEncodeDstKHR";
        case eVideoEncodeSrcKHR:                       return "VideoEncodeSrcKHR";
        case eVideoEncodeDpbKHR:                       return "VideoEncodeDpbKHR";
        case eAttachmentFeedbackLoopOptimalEXT:        return "AttachmentFeedbackLoopOptimalEXT";
        case eVideoEncodeQuantizationMapKHR:           return "VideoEncodeQuantizationMapKHR";
        default:                                       return "Unknown";
    }
}

std::string tpd::utils::toString(const vk::Format format) {
    using enum vk::Format;
    switch (format) {
        // Undefined format
        case eUndefined: return "Undefined";

        // 8-bit normalized formats
        case eR8Unorm:   return "R8 Unorm";
        case eR8Snorm:   return "R8 Snorm";
        case eR8Uscaled: return "R8 Uscaled";
        case eR8Sscaled: return "R8 Sscaled";
        case eR8Uint:    return "R8 Uint";
        case eR8Sint:    return "R8 Sint";
        case eR8Srgb:    return "R8 SRGB";

        // 16-bit formats
        case eR16Unorm:   return "R16 Unorm";
        case eR16Snorm:   return "R16 Snorm";
        case eR16Uscaled: return "R16 Uscaled";
        case eR16Sscaled: return "R16 Sscaled";
        case eR16Uint:    return "R16 Uint";
        case eR16Sint:    return "R16 Sint";
        case eR16Sfloat:  return "R16 Sfloat";

        // 32-bit formats
        case eR32Uint:   return "R32 Uint";
        case eR32Sint:   return "R32 Sint";
        case eR32Sfloat: return "R32 Sfloat";

        // 64-bit formats
        case eR64Uint:   return "R64 Uint";
        case eR64Sint:   return "R64 Sint";
        case eR64Sfloat: return "R64 Sfloat";

        // RGBA 8-bit formats
        case eR8G8B8A8Unorm:   return "RGBA8 Unorm";
        case eR8G8B8A8Snorm:   return "RGBA8 Snorm";
        case eR8G8B8A8Uscaled: return "RGBA8 Uscaled";
        case eR8G8B8A8Sscaled: return "RGBA8 Sscaled";
        case eR8G8B8A8Uint:    return "RGBA8 Uint";
        case eR8G8B8A8Sint:    return "RGBA8 Sint";
        case eR8G8B8A8Srgb:    return "RGBA8 SRGB";

        // BGRA formats
        case eB8G8R8A8Unorm: return "BGRA8 Unorm";
        case eB8G8R8A8Srgb:  return "BGRA8 SRGB";

        // RGBA 16-bit formats
        case eR16G16B16A16Unorm:   return "RGBA16 Unorm";
        case eR16G16B16A16Snorm:   return "RGBA16 Snorm";
        case eR16G16B16A16Uscaled: return "RGBA16 Uscaled";
        case eR16G16B16A16Sscaled: return "RGBA16 Sscaled";
        case eR16G16B16A16Uint:    return "RGBA16 Uint";
        case eR16G16B16A16Sint:    return "RGBA16 Sint";
        case eR16G16B16A16Sfloat:  return "RGBA16 Sfloat";

        // RGBA 32-bit formats
        case eR32G32B32A32Uint:   return "RGBA32 Uint";
        case eR32G32B32A32Sint:   return "RGBA32 Sint";
        case eR32G32B32A32Sfloat: return "RGBA32 Sfloat";

        // Depth-stencil formats
        case eD16Unorm:         return "D16 Unorm";
        case eX8D24UnormPack32: return "X8 D24 Unorm Pack32";
        case eD32Sfloat:        return "D32 Sfloat";
        case eS8Uint:           return "S8 Uint";
        case eD16UnormS8Uint:   return "D16 Unorm + S8 Uint";
        case eD24UnormS8Uint:   return "D24 Unorm + S8 Uint";
        case eD32SfloatS8Uint:  return "D32 Sfloat + S8 Uint";

        // Compressed formats (BC)
        case eBc1RgbUnormBlock:  return "BC1 RGB Unorm";
        case eBc1RgbSrgbBlock:   return "BC1 RGB SRGB";
        case eBc1RgbaUnormBlock: return "BC1 RGBA Unorm";
        case eBc1RgbaSrgbBlock:  return "BC1 RGBA SRGB";
        case eBc2UnormBlock:     return "BC2 Unorm";
        case eBc2SrgbBlock:      return "BC2 SRGB";
        case eBc3UnormBlock:     return "BC3 Unorm";
        case eBc3SrgbBlock:      return "BC3 SRGB";

        // Compressed formats (ETC)
        case eEtc2R8G8B8UnormBlock:   return "ETC2 RGB Unorm";
        case eEtc2R8G8B8SrgbBlock:    return "ETC2 RGB SRGB";
        case eEtc2R8G8B8A1UnormBlock: return "ETC2 RGB+A1 Unorm";
        case eEtc2R8G8B8A1SrgbBlock:  return "ETC2 RGB+A1 SRGB";

        // ASTC formats
        case eAstc4x4UnormBlock: return "ASTC 4x4 Unorm";
        case eAstc4x4SrgbBlock:  return "ASTC 4x4 SRGB";
        case eAstc8x8UnormBlock: return "ASTC 8x8 Unorm";
        case eAstc8x8SrgbBlock:  return "ASTC 8x8 SRGB";

        default: return "Unknown";
    }
}
