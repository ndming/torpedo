#pragma once

#include "torpedo/foundation/Buffer.h"

namespace tpd {
    class RingBuffer final : public Buffer {
    public:
        class Builder {
        public:
            Builder& count(uint32_t bufferCount) noexcept;
            Builder& usage(vk::BufferUsageFlags usage) noexcept;
            Builder& alloc(std::size_t byteSize, Alignment alignment = Alignment::None) noexcept;

            [[nodiscard]] RingBuffer build(const DeviceAllocator& allocator) const;

            [[nodiscard]] std::unique_ptr<RingBuffer, Deleter<RingBuffer>> build(
                const DeviceAllocator& allocator, std::pmr::memory_resource* pool) const;

        private:
            uint32_t _bufferCount{};
            vk::BufferUsageFlags _usage{};
            std::size_t _bufferSize{};
            std::size_t _allocSize{};
        };

        RingBuffer(
            std::byte* pMappedData, uint32_t bufferCount, std::size_t bufferSize, std::size_t allocSize,
            vk::Buffer buffer, VmaAllocation allocation, const DeviceAllocator& allocator);

        RingBuffer(const RingBuffer&) = delete;
        RingBuffer& operator=(const RingBuffer&) = delete;

        void updateData(uint32_t bufferIndex, const void* data, std::size_t byteSize = 0) const;
        void updateData(const void* data, std::size_t byteSize = 0) const;

        void destroy() noexcept override;
        ~RingBuffer() noexcept override;

    private:
        std::byte* _pMappedData;
        uint32_t   _bufferCount;
        std::size_t _perBufferSize;
        std::size_t _perAllocSize;
    };
}

// =========================== //
// INLINE FUNCTION DEFINITIONS //
// =========================== //

inline tpd::RingBuffer::Builder& tpd::RingBuffer::Builder::count(const uint32_t bufferCount) noexcept {
    _bufferCount = bufferCount;
    return *this;
}

inline tpd::RingBuffer::Builder& tpd::RingBuffer::Builder::usage(const vk::BufferUsageFlags usage) noexcept {
    _usage = usage;
    return *this;
}

inline tpd::RingBuffer::Builder& tpd::RingBuffer::Builder::alloc(const std::size_t byteSize, const Alignment alignment) noexcept {
    _bufferSize = byteSize;
    _allocSize  = foundation::getAlignedSize(byteSize, alignment);
    return *this;
}

inline tpd::RingBuffer::RingBuffer(
    std::byte* pMappedData,
    const uint32_t bufferCount,
    const std::size_t bufferSize,
    const std::size_t allocSize,
    const vk::Buffer buffer,
    VmaAllocation allocation,
    const DeviceAllocator& allocator)
    : Buffer{ buffer, allocation, allocator }
    , _pMappedData{ pMappedData }
    , _bufferCount{ bufferCount }
    , _perBufferSize{ bufferSize }
    , _perAllocSize{ allocSize } {
}

inline void tpd::RingBuffer::destroy() noexcept {
    _pMappedData = nullptr;
    _bufferCount = 0;
    Buffer::destroy();
}

inline tpd::RingBuffer::~RingBuffer() noexcept {
    destroy();
}
