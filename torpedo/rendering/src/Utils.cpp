#include "torpedo/rendering/Utils.h"

#include <plog/Log.h>

#include <format>

void tpd::rendering::logDebugExtensions(
    const std::string_view extensionType,
    const std::string_view className,
    const std::vector<const char*>& extensions)
{
    const auto terminateString = extensions.empty() ? " (none)" : std::format(" ({}):", extensions.size());
    PLOGD << extensionType << " extensions required by " << className << terminateString;
    for (const auto& extension : extensions) {
        PLOGD << " - " << extension;
    }
}

std::string tpd::rendering::formatDriverVersion(const uint32_t version) {
    const auto major  = version >> 22 & 0x3FF;  // 10 bits
    const auto minor  = version >> 12 & 0x3FF;  // 10 bits
    const auto patch  = version >> 4  &  0xFF;  //  8 bits
    const auto branch = version       &  0xFF;  //  4 bits
    return std::format("{}.{}.{}.{}", major, minor, patch, branch);
}

std::string tpd::rendering::toString(const vk::ColorSpaceKHR colorSpace) {
    using enum vk::ColorSpaceKHR;
    switch (colorSpace) {
        case eSrgbNonlinear:            return "sRGB Non Linear";
        case eDisplayP3NonlinearEXT:    return "Display P3 Non Linear (EXT)";
        case eExtendedSrgbLinearEXT:    return "Extended sRGB Linear (EXT)";
        case eDisplayP3LinearEXT:       return "Display P3 Linear (EXT)";
        case eDciP3NonlinearEXT:        return "DCI P3 Non Linear (EXT)";
        case eBt709LinearEXT:           return "BT.709 Linear (EXT)";
        case eBt709NonlinearEXT:        return "BT.709 Non Linear (EXT)";
        case eBt2020LinearEXT:          return "BT.2020 Linear (EXT)";
        case eHdr10St2084EXT:           return "HDR10 ST2084 (EXT)";
        case eHdr10HlgEXT:              return "HDR10 HLG (EXT)";
        case eAdobergbLinearEXT:        return "Adobe RGB Linear (EXT)";
        case eAdobergbNonlinearEXT:     return "Adobe RGB Non Linear (EXT)";
        case ePassThroughEXT:           return "Pass Through (EXT)";
        case eExtendedSrgbNonlinearEXT: return "Extended sRGB Non Linear (EXT)";
        case eDisplayNativeAMD:         return "Display Native (AMD)";
        default:                        return "Unknown or deprecated color space";
    }
}

std::string tpd::rendering::toString(const vk::Extent2D extent) {
    return "(" + std::to_string(extent.width) + ", " + std::to_string(extent.height) + ")";
}

std::string tpd::rendering::toString(const vk::PresentModeKHR presentMode) {
    using enum vk::PresentModeKHR;
    switch (presentMode) {
    case eImmediate:               return "Immediate";
    case eMailbox:                 return "Mailbox";
    case eFifo :                   return "Fifo";
    case eFifoRelaxed:             return "FifoRelaxed";
    case eSharedDemandRefresh:     return "SharedDemandRefresh";
    case eSharedContinuousRefresh: return "SharedContinuousRefresh";
    default: return "Unrecognized present mode at enum: " + std::to_string(static_cast<int>(presentMode));
    }
}
