#pragma once

#include "torpedo/foundation/Buffer.h"

namespace tpd {
    class StorageBuffer final : public Buffer {
    public:
        class Builder final : public Buffer::Builder<Builder, StorageBuffer> {
        public:
            [[nodiscard]] StorageBuffer build(VmaAllocator allocator) const override;
        };

        StorageBuffer() noexcept = default;
        StorageBuffer(vk::Buffer buffer, VmaAllocation allocation);

        void recordStagingCopy(vk::CommandBuffer cmd, vk::Buffer stagingBuffer, vk::DeviceSize size) const noexcept;
        void recordTransferDst(vk::CommandBuffer cmd, SyncPoint dstSync) const noexcept;
    };
} // namespace tpd

inline tpd::StorageBuffer::StorageBuffer(const vk::Buffer buffer, VmaAllocation allocation) 
    : Buffer{ buffer, allocation } {}
