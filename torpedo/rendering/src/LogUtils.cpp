#include "torpedo/rendering/LogUtils.h"

#include <plog/Init.h>
#include <plog/Util.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Record.h>

#include <format>
#include <iomanip>

void tpd::utils::logDebugExtensions(
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

std::string tpd::utils::formatDriverVersion(const uint32_t version) {
    const auto major  = version >> 22 & 0x3FF;  // 10 bits
    const auto minor  = version >> 12 & 0x3FF;  // 10 bits
    const auto patch  = version >> 4  &  0xFF;  //  8 bits
    const auto branch = version       &  0xFF;  //  4 bits
    return std::format("{}.{}.{}.{}", major, minor, patch, branch);
}

std::string tpd::utils::toString(const vk::ColorSpaceKHR colorSpace) {
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

std::string tpd::utils::toString(const vk::Extent2D extent) {
    return "(" + std::to_string(extent.width) + ", " + std::to_string(extent.height) + ")";
}

std::string tpd::utils::toString(const vk::PresentModeKHR presentMode) {
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

namespace plog {
    class Formatter {
    public:
        static util::nstring header() {
            return {};
        }

        static util::nstring format(const Record& record) {
            tm t{};
            util::localtime_s(&t, &record.getTime().time);  // using local time

            util::nostringstream ss;
            ss << t.tm_year + 1900 << "-" << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_mon + 1 << PLOG_NSTR("-") << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_mday << PLOG_NSTR(" ");
            ss << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_hour << PLOG_NSTR(":") << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_min << PLOG_NSTR(":") << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_sec << PLOG_NSTR(".") << std::setfill(PLOG_NSTR('0')) << std::setw(3) << static_cast<int> (record.getTime().millitm) << PLOG_NSTR(" ");
            ss << std::setfill(PLOG_NSTR(' ')) << std::setw(5) << std::left << severityToString(record.getSeverity()) << PLOG_NSTR(" ");
            ss << PLOG_NSTR("[") << std::setfill(PLOG_NSTR(' ')) << std::setw(5) << std::left << record.getTid() << PLOG_NSTR("] ");
            ss << record.getMessage() << PLOG_NSTR("\n");

            return ss.str();
        }
    };
}

void tpd::utils::plantConsoleLogger() {
    static auto appender = plog::ColorConsoleAppender<plog::Formatter>();
#ifdef NDEBUG
    init(plog::info, &appender);
#else
    init(plog::debug, &appender);
#endif
}

void tpd::utils::logVerbose(const std::string_view message) {
    PLOGV << message.data();
}

void tpd::utils::logInfo(const std::string_view message) {
    PLOGI << message.data();
}

void tpd::utils::logDebug(const std::string_view message) {
    PLOGD << message.data();
}

void tpd::utils::logError(const std::string_view message) {
    PLOGE << message.data();
}
