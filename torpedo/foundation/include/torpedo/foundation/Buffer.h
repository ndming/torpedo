#pragma once

#include "torpedo/foundation/DeviceAllocator.h"

namespace tpd {
    class Buffer : public Destroyable {
    public:
        Buffer(vk::Buffer buffer, VmaAllocation allocation, const DeviceAllocator& allocator);

        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;

        [[nodiscard]] vk::Buffer getVulkanBuffer() const noexcept;

        void destroy() noexcept override;
        ~Buffer() noexcept override;

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

inline vk::Buffer tpd::Buffer::getVulkanBuffer() const noexcept {
    return _buffer;
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
