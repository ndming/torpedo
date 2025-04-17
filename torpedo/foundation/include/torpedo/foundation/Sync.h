#pragma once

#include <vulkan/vulkan.hpp>

namespace tpd {
    struct SyncPoint {
        vk::PipelineStageFlags2 stage{};
        vk::AccessFlags2 access{};
    };

} // namespace tpd

namespace tpd::sync {
    [[nodiscard]] constexpr SyncPoint storageBufferTransferOwnershipReleaseSrcSync() noexcept {
        return { vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite };
    }
}
