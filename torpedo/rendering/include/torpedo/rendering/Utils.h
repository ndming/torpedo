#pragma once

#include <vulkan/vulkan.hpp>

namespace tpd::rendering {
    void logDebugExtensions(std::string_view extensionType, std::string_view className, const std::vector<const char*>& extensions = {});

    [[nodiscard]] std::string formatDriverVersion(uint32_t version);

    [[nodiscard]] std::string toString(vk::Extent2D extent);
    [[nodiscard]] std::string toString(vk::PresentModeKHR presentMode);
}
