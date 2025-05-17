#pragma once

#include "torpedo/foundation/Buffer.h"

namespace tpd {
    class TwoWayBuffer final : public Buffer {
    public:
        class Builder final : public Buffer::Builder<Builder, TwoWayBuffer> {
        public:
            [[nodiscard]] TwoWayBuffer build(VmaAllocator allocator) const override;
        };

        TwoWayBuffer() noexcept = default;
        TwoWayBuffer(void* pMappedData, vk::Buffer buffer, VmaAllocation allocation);

        TwoWayBuffer(TwoWayBuffer&& other) noexcept;
        TwoWayBuffer& operator=(TwoWayBuffer&& other) noexcept;

        template<typename T>
        [[nodiscard]] const T* data() const noexcept;

        template<typename T> requires (std::is_trivially_copyable_v<T>)
        [[nodiscard]] T read() const noexcept;

        template<typename T> requires (std::is_trivially_copyable_v<T>)
        void write(T value) const noexcept;

        template<typename T>
        [[nodiscard]] std::span<const T> read(uint32_t count) const noexcept;

        [[nodiscard]] bool mapped() const noexcept;

    private:
        void* _pMappedData{ nullptr };
    };
} // namespace tpd

inline tpd::TwoWayBuffer::TwoWayBuffer(void* pMappedData, const vk::Buffer buffer, VmaAllocation allocation)
    : Buffer{ buffer, allocation }, _pMappedData{ pMappedData }
{}

inline tpd::TwoWayBuffer::TwoWayBuffer(TwoWayBuffer&& other) noexcept
    : Buffer{ std::move(other) }, _pMappedData{ other._pMappedData } {
    other._pMappedData = nullptr;
}

inline tpd::TwoWayBuffer& tpd::TwoWayBuffer::operator=(TwoWayBuffer&& other) noexcept {
    if (this == &other || valid()) {
        return *this;
    }
    Buffer::operator=(std::move(other));

    _pMappedData = other._pMappedData;
    other._pMappedData = nullptr;

    return *this;
}

template<typename T>
const T* tpd::TwoWayBuffer::data() const noexcept {
    return static_cast<const T*>(_pMappedData);
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
T tpd::TwoWayBuffer::read() const noexcept {
    return *static_cast<const T*>(_pMappedData);
}

template<typename T> requires (std::is_trivially_copyable_v<T>)
void tpd::TwoWayBuffer::write(const T value) const noexcept {
    memcpy(_pMappedData, &value, sizeof(T));
}

template<typename T>
std::span<const T> tpd::TwoWayBuffer::read(const uint32_t count) const noexcept {
    return std::span{ static_cast<const T*>(_pMappedData), count };
}

inline bool tpd::TwoWayBuffer::mapped() const noexcept {
    return _pMappedData != nullptr;
}
