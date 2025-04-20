#pragma once

#include <vulkan/vulkan.hpp>

namespace tpd::utils {
    [[nodiscard]] std::string toString(vk::ImageLayout layout);
    [[nodiscard]] std::string toString(vk::Format format);
} // namespace tpd::utils
