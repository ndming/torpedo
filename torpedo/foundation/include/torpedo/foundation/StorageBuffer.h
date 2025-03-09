#pragma once

#include "torpedo/foundation/Buffer.h"

namespace tpd {
    class StorageBuffer final : public Buffer {
    public:
        class Builder {
        public:
            Builder& usage(vk::BufferUsageFlags usage) noexcept;
            Builder& alloc(std::size_t byteSize, std::size_t alignment = 0) noexcept;

            [[nodiscard]] StorageBuffer build(const DeviceAllocator& allocator) const;

            [[nodiscard]] std::unique_ptr<StorageBuffer, foundation::Deleter<StorageBuffer>> build(
                const DeviceAllocator& allocator, std::pmr::memory_resource* pool) const;

        private:
            [[nodiscard]] vk::Buffer creatBuffer(const DeviceAllocator& allocator, VmaAllocation* allocation) const;

            vk::BufferUsageFlags _usage{};
            std::size_t _allocSize{};
        };

        StorageBuffer(vk::Buffer buffer, VmaAllocation allocation, const DeviceAllocator& allocator);

        StorageBuffer(const StorageBuffer&) = delete;
        StorageBuffer& operator=(const StorageBuffer&) = delete;
    };
}

// =========================== //
// INLINE FUNCTION DEFINITIONS //
// =========================== //

inline tpd::StorageBuffer::Builder& tpd::StorageBuffer::Builder::usage(const vk::BufferUsageFlags usage) noexcept {
    _usage = usage;
    return *this;
}

inline tpd::StorageBuffer::Builder& tpd::StorageBuffer::Builder::alloc(const std::size_t byteSize, const std::size_t alignment) noexcept {
    _allocSize = foundation::getAlignedSize(byteSize, alignment);
    return *this;
}

inline tpd::StorageBuffer::StorageBuffer(const vk::Buffer buffer, VmaAllocation allocation, const DeviceAllocator& allocator)
    : Buffer{ buffer, allocation, allocator } {
}
