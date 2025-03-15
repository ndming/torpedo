#pragma once

#include <vulkan/vulkan.hpp>

namespace tpd::foundation {
    void recordLayoutTransition(
        vk::CommandBuffer cmd,
        vk::Image image,
        vk::ImageAspectFlags aspectMask,
        uint32_t mipLevelCount,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout);

    [[nodiscard]] std::string toString(vk::ImageLayout layout);
    [[nodiscard]] std::string toString(vk::Format format);
}
