#pragma once

#include "torpedo/foundation/Buffer.h"
#include "torpedo/foundation/SyncResource.h"

namespace tpd {
    class StorageBuffer final : public Buffer, public SyncResource {
    public:
        class Builder {
        public:
            Builder& usage(vk::BufferUsageFlags usage) noexcept;
            Builder& alloc(std::size_t byteSize, Alignment alignment = Alignment::None) noexcept;

            [[nodiscard]] StorageBuffer build(const DeviceAllocator& allocator) const;

            [[nodiscard]] std::unique_ptr<StorageBuffer, Deleter<StorageBuffer>> build(
                const DeviceAllocator& allocator, std::pmr::memory_resource* pool) const;

        private:
            [[nodiscard]] vk::Buffer creatBuffer(const DeviceAllocator& allocator, VmaAllocation* allocation) const;

            vk::BufferUsageFlags _usage{};
            std::size_t _allocSize{};
        };

        StorageBuffer(vk::Buffer buffer, VmaAllocation allocation, const DeviceAllocator& allocator);

        StorageBuffer(const StorageBuffer&) = delete;
        StorageBuffer& operator=(const StorageBuffer&) = delete;

        void recordBufferTransfer(vk::CommandBuffer cmd, vk::Buffer stagingBuffer) const noexcept;

        void recordOwnershipRelease(vk::CommandBuffer cmd, uint32_t srcFamilyIndex, uint32_t dstFamilyIndex) const noexcept override;
        void recordOwnershipAcquire(vk::CommandBuffer cmd, uint32_t srcFamilyIndex, uint32_t dstFamilyIndex) const noexcept override;
    };
}

// =========================== //
// INLINE FUNCTION DEFINITIONS //
// =========================== //

inline tpd::StorageBuffer::Builder& tpd::StorageBuffer::Builder::usage(const vk::BufferUsageFlags usage) noexcept {
    _usage = usage;
    return *this;
}

inline tpd::StorageBuffer::Builder& tpd::StorageBuffer::Builder::alloc(const std::size_t byteSize, const Alignment alignment) noexcept {
    _allocSize = foundation::getAlignedSize(byteSize, alignment);
    return *this;
}

inline tpd::StorageBuffer::StorageBuffer(const vk::Buffer buffer, VmaAllocation allocation, const DeviceAllocator& allocator)
    : Buffer{ buffer, allocation, allocator } {
}
