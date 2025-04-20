#pragma once

#include <vulkan/vulkan.hpp>

#include <plog/Log.h>

namespace tpd::utils {
    void logDebugExtensions(std::string_view extensionType, std::string_view className, const std::vector<const char*>& extensions = {});

    [[nodiscard]] std::string formatDriverVersion(uint32_t version);

    [[nodiscard]] std::string toString(vk::ColorSpaceKHR colorSpace);
    [[nodiscard]] std::string toString(vk::Extent2D extent);
    [[nodiscard]] std::string toString(vk::PresentModeKHR presentMode);

    void plantConsoleLogger();

    void logVerbose(std::string_view message);
    void logInfo(std::string_view message);
    void logDebug(std::string_view message);
    void logError(std::string_view message);
}  // namespace tpd::rendering
