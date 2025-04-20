#pragma once

#include "torpedo/foundation/Buffer.h"

namespace tpd {
    class ReadbackBuffer final : public Buffer {
    public:
        class Builder final : public Buffer::Builder<Builder, ReadbackBuffer> {
        public:
            [[nodiscard]] ReadbackBuffer build(VmaAllocator allocator) const override;
        };

        ReadbackBuffer() noexcept = default;
        ReadbackBuffer(void* pMappedData, vk::Buffer buffer, VmaAllocation allocation);

        ReadbackBuffer(ReadbackBuffer&& other) noexcept;
        ReadbackBuffer& operator=(ReadbackBuffer&& other) noexcept;

        template<typename T>
        [[nodiscard]] const T* data() const noexcept;

        template<typename T> requires (std::is_trivially_copyable_v<T>)
        [[nodiscard]] T read() const noexcept;

        template<typename T>
        [[nodiscard]] std::span<const T> read(uint32_t count) const noexcept;

        [[nodiscard]] bool mapped() const noexcept;

    private:
        void* _pMappedData{ nullptr };
    };
} // namespace tpd

inline tpd::ReadbackBuffer::ReadbackBuffer(void* pMappedData, const vk::Buffer buffer, VmaAllocation allocation)
    : Buffer{ buffer, allocation }, _pMappedData{ pMappedData }
{}

inline tpd::ReadbackBuffer::ReadbackBuffer(ReadbackBuffer&& other) noexcept
    : Buffer{ std::move(other) }, _pMappedData{ other._pMappedData } {
    other._pMappedData = nullptr;
}

inline tpd::ReadbackBuffer& tpd::ReadbackBuffer::operator=(ReadbackBuffer&& other) noexcept {
    if (this == &other || valid()) {
        return *this;
    }
    Buffer::operator=(std::move(other));

    _pMappedData = other._pMappedData;
    other._pMappedData = nullptr;

    return *this;
}

template<typename T>
const T* tpd::ReadbackBuffer::data() const noexcept {
    return static_cast<const T*>(_pMappedData);
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
T tpd::ReadbackBuffer::read() const noexcept {
    return *static_cast<const T*>(_pMappedData);
}

template<typename T>
std::span<const T> tpd::ReadbackBuffer::read(const uint32_t count) const noexcept {
    return std::span{ static_cast<const T*>(_pMappedData), count };
}

inline bool tpd::ReadbackBuffer::mapped() const noexcept {
    return _pMappedData != nullptr;
}
