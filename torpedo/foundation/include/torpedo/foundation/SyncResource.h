#pragma once

#include <vulkan/vulkan.hpp>

namespace tpd {
    class SyncResource {
    public:
        SyncResource(const SyncResource&) = delete;
        SyncResource& operator=(const SyncResource&) = delete;

        void setSyncData(const void* data, uint32_t byteSize) noexcept;
        [[nodiscard]] bool hasSyncData() const noexcept;

        [[nodiscard]] const std::byte* getSyncData() const noexcept;
        [[nodiscard]] uint32_t getSyncDataSize() const noexcept;

        virtual void recordOwnershipRelease(vk::CommandBuffer cmd, uint32_t srcFamilyIndex, uint32_t dstFamilyIndex) const noexcept = 0;
        virtual void recordOwnershipAcquire(vk::CommandBuffer cmd, uint32_t srcFamilyIndex, uint32_t dstFamilyIndex) const noexcept = 0;

        virtual ~SyncResource() = default;

    protected:
        explicit SyncResource(const void* data = nullptr, const uint32_t byteSize = 0)
            : _data{ static_cast<const std::byte*>(data) }, _size{ byteSize } {
        }

    private:
        const std::byte* _data;
        uint32_t _size;
    };
}
