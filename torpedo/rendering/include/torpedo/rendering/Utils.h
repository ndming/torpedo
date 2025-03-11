#pragma once

#include <string_view>
#include <vector>

namespace tpd::rendering {
    void logExtensions(std::string_view extensionType, std::string_view className, const std::vector<const char*>& extensions = {});
}
