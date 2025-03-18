#pragma once

#include <plog/Log.h>

namespace tpd::utils {
    void plantConsoleLogger();

    inline void logVerbose(const std::string_view message) {
        PLOGV << message.data();
    }

    inline void logInfo(const std::string_view message) {
        PLOGI << message.data();
    }

    inline void logDebug(const std::string_view message) {
        PLOGD << message.data();
    }

    inline void logError(const std::string_view message) {
        PLOGE << message.data();
    }
}
