#pragma once

#include "torpedo/foundation/Buffer.h"

namespace tpd {
    class StagingBuffer final : public Buffer {
    public:
        class Builder {
        public:
            Builder& alloc(std::size_t byteSize, std::size_t alignment = 0) noexcept;

            [[nodiscard]] StagingBuffer build(const DeviceAllocator& allocator) const;

            [[nodiscard]] std::unique_ptr<StagingBuffer, Deleter<StagingBuffer>> build(
                const DeviceAllocator& allocator, std::pmr::memory_resource* pool) const;

        private:
            std::size_t _bufferSize{};
            std::size_t _allocSize{};
        };

        StagingBuffer(std::size_t size, vk::Buffer buffer, VmaAllocation allocation, const DeviceAllocator& allocator);

        void setData(const void* data, std::size_t byteSize = 0) const;
    };
} // namespace tpd

inline tpd::StagingBuffer::Builder& tpd::StagingBuffer::Builder::alloc(const std::size_t byteSize, const std::size_t alignment) noexcept {
    _bufferSize = byteSize;
    _allocSize  = foundation::getAlignedSize(byteSize, alignment);
    return *this;
}

inline tpd::StagingBuffer::StagingBuffer(
    const std::size_t size,
    const vk::Buffer buffer,
    VmaAllocation allocation,
    const DeviceAllocator& allocator)
    : Buffer{ size, buffer, allocation, allocator }
{}
