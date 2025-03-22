#include "torpedo/rendering/Utils.h"

#include <plog/Log.h>

#include <format>

void tpd::rendering::logExtensions(
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
    const auto major  = (version >> 22) & 0x3FF;  // 10 bits
    const auto minor  = (version >> 12) & 0x3FF;  // 10 bits
    const auto patch  = (version >> 4) & 0xFF;    //  8 bits
    const auto branch = (version & 0xFF);         //  4 bits
    return std::format("{}.{}.{}.{}", major, minor, patch, branch);
}
