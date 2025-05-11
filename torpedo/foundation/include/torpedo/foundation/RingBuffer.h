#pragma once

#include "torpedo/foundation/Buffer.h"

namespace tpd {
    class RingBuffer final : public Buffer {
    public:
        class Builder final : public Buffer::Builder<Builder, RingBuffer> {
        public:
            Builder& count(uint32_t bufferCount) noexcept;

            [[nodiscard]] RingBuffer build(VmaAllocator allocator) const override;

        private:
            uint32_t _bufferCount{};
        };

        RingBuffer() noexcept = default;

        RingBuffer(
            std::byte* pMappedData, uint32_t bufferCount, uint32_t allocSizePerBuffer,
            vk::Buffer buffer, VmaAllocation allocation);

        RingBuffer(RingBuffer&& other) noexcept;
        RingBuffer& operator=(RingBuffer&& other) noexcept;

        void update(uint32_t bufferIndex, const void* data, std::size_t size, std::size_t offset = 0) const;
        void update(const void* data, std::size_t size, std::size_t offset = 0) const;

        [[nodiscard]] uint32_t getOffset(uint32_t bufferIndex) const;

    private:
        std::byte* _pMappedData{ nullptr };
        uint32_t _bufferCount{ 0 };
        uint32_t _allocSizePerBuffer{ 0 };
    };
} // namespace tpd

inline tpd::RingBuffer::Builder& tpd::RingBuffer::Builder::count(const uint32_t bufferCount) noexcept {
    _bufferCount = bufferCount;
    return *this;
}

inline tpd::RingBuffer::RingBuffer(
    std::byte* pMappedData,
    const uint32_t bufferCount,
    const uint32_t allocSizePerBuffer,
    const vk::Buffer buffer,
    VmaAllocation allocation)
    : Buffer{ buffer, allocation }
    , _pMappedData{ pMappedData }
    , _bufferCount{ bufferCount }
    , _allocSizePerBuffer{ allocSizePerBuffer }
{}

inline tpd::RingBuffer::RingBuffer(RingBuffer&& other) noexcept
    : Buffer{ std::move(other) }
    , _pMappedData{ other._pMappedData }
    , _bufferCount{ other._bufferCount }
    , _allocSizePerBuffer{ other._allocSizePerBuffer }
{
    other._pMappedData = nullptr;
    other._bufferCount = 0;
    other._allocSizePerBuffer = 0;
}

inline tpd::RingBuffer& tpd::RingBuffer::operator=(RingBuffer&& other) noexcept {
    if (this == &other || valid()) {
        return *this;
    }
    Buffer::operator=(std::move(other));
    _pMappedData = other._pMappedData;
    _bufferCount = other._bufferCount;
    _allocSizePerBuffer = other._allocSizePerBuffer;

    other._pMappedData = nullptr;
    other._bufferCount = 0;
    other._allocSizePerBuffer = 0;
    return *this;
}
