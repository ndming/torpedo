#pragma once

#include <string_view>

namespace tpd::utils {
    void plantConsoleLogger();

    void logVerbose(std::string_view message);
    void logInfo(std::string_view message);
    void logDebug(std::string_view message);
    void logError(std::string_view message);
}
