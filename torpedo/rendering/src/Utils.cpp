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
