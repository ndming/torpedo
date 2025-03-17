#pragma once

#include <vulkan/vulkan.hpp>

namespace tpd {
    class SyncResource {
    public:
        SyncResource(const SyncResource&) = delete;
        SyncResource& operator=(const SyncResource&) = delete;

        void setSyncData(const void* data, uint32_t byteSize) noexcept;
        bool hasSyncData() const noexcept;

        [[nodiscard]] const std::byte* getSyncData() const noexcept;
        [[nodiscard]] uint32_t getSyncDataSize() const noexcept;

        virtual void recordOwnershipRelease(vk::CommandBuffer cmd, uint32_t srcFamilyIndex, uint32_t dstFamilyIndex) const noexcept = 0;
        virtual void recordOwnershipAcquire(vk::CommandBuffer cmd, uint32_t srcFamilyIndex, uint32_t dstFamilyIndex) const noexcept = 0;

        virtual ~SyncResource() = default;

    protected:
        SyncResource() = default;

    private:
        const std::byte* _data{ nullptr };
        uint32_t _size{ 0 };
    };
}
