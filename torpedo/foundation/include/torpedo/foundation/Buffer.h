#pragma once

#include "torpedo/foundation/DeviceAllocator.h"

namespace tpd {
    class Buffer {
    public:
        Buffer(vk::Buffer buffer, VmaAllocation allocation, const DeviceAllocator& allocator);

        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;

        [[nodiscard]] vk::Buffer getBuffer() const { return _buffer; }

        virtual void destroy() noexcept;
        virtual ~Buffer() noexcept;

    protected:
        vk::Buffer _buffer;
        VmaAllocation _allocation;
        const DeviceAllocator& _allocator;
    };
}

// =========================== //
// INLINE FUNCTION DEFINITIONS //
// =========================== //

inline tpd::Buffer::Buffer(const vk::Buffer buffer, VmaAllocation allocation, const DeviceAllocator& allocator)
    : _buffer{ buffer }, _allocation{ allocation }, _allocator{ allocator } {
    if (!buffer || !allocation) [[unlikely]] {
        throw std::invalid_argument("Buffer - vk::Buffer is in invalid state: consider using a builder");
    }
}

inline void tpd::Buffer::destroy() noexcept {
    if (_allocation) {
        _allocator.deallocateBuffer(_buffer, _allocation);
        _allocation = nullptr;
    }
}

inline tpd::Buffer::~Buffer() noexcept {
    Buffer::destroy();
}
