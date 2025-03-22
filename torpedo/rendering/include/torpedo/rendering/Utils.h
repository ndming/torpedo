#pragma once

#include <string>
#include <vector>

namespace tpd::rendering {
    void logExtensions(std::string_view extensionType, std::string_view className, const std::vector<const char*>& extensions = {});

    [[nodiscard]] std::string formatDriverVersion(uint32_t version);
}
