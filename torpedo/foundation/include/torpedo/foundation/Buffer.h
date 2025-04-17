#pragma once

#include "torpedo/foundation/VmaUsage.h"

namespace tpd {
    class Buffer {
    public:
        Buffer(std::size_t size, vk::Buffer buffer, VmaAllocation allocation);

        Buffer(Buffer&& other) noexcept;
        Buffer& operator=(Buffer&& other) noexcept;

        [[nodiscard]] vk::Buffer get() const noexcept;
        [[nodiscard]] std::size_t getSize() const noexcept;

        void destroy(VmaAllocator allocator) noexcept;

    protected:
        std::size_t _size;
        vk::Buffer _buffer;
        VmaAllocation _allocation;
    };
} // namespace tpd

inline tpd::Buffer::Buffer(const std::size_t size, const vk::Buffer buffer, VmaAllocation allocation)
    : _size{ size }
    , _buffer{ buffer }
    , _allocation { allocation }
{
    if (!buffer || !allocation) [[unlikely]] {
        throw std::invalid_argument("Buffer - vk::Buffer is in invalid state: consider using a builder");
    }
}

inline tpd::Buffer::Buffer(Buffer&& other) noexcept
    : _size{ other._size }
    , _buffer{ other._buffer }
    , _allocation{ other._allocation }
{
    other._size = 0;
    other._buffer = nullptr;
    other._allocation = nullptr;
}

inline vk::Buffer tpd::Buffer::get() const noexcept {
    return _buffer;
}

inline std::size_t tpd::Buffer::getSize() const noexcept {
    return _size;
}

inline void tpd::Buffer::destroy(VmaAllocator allocator) noexcept {
    if (_allocation) {
        _size = 0;
        vma::deallocateBuffer(allocator, _buffer, _allocation);
        _allocation = nullptr;
    }
}
