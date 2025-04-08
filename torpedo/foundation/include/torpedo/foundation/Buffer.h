#pragma once

#include "torpedo/foundation/DeviceAllocator.h"

namespace tpd {
    class Buffer : public Destroyable {
    public:
        Buffer(std::size_t size, vk::Buffer buffer, VmaAllocation allocation, const DeviceAllocator& allocator);

        [[nodiscard]] vk::Buffer getVulkanBuffer() const noexcept;
        [[nodiscard]] std::size_t getSize() const noexcept;

        void destroy() noexcept override;
        ~Buffer() noexcept override;

    protected:
        std::size_t _size;
        vk::Buffer _buffer;
    };
} // namespace tpd

inline tpd::Buffer::Buffer(const std::size_t size, const vk::Buffer buffer, VmaAllocation allocation, const DeviceAllocator& allocator)
    : Destroyable{ allocation, allocator }, _size{ size }, _buffer{ buffer } {
    if (!buffer || !allocation) [[unlikely]] {
        throw std::invalid_argument("Buffer - vk::Buffer is in invalid state: consider using a builder");
    }
}

inline vk::Buffer tpd::Buffer::getVulkanBuffer() const noexcept {
    return _buffer;
}

inline std::size_t tpd::Buffer::getSize() const noexcept {
    return _size;
}

inline void tpd::Buffer::destroy() noexcept {
    if (_allocation) {
        _allocator.deallocateBuffer(_buffer, _allocation);
    }
    Destroyable::destroy();
}

inline tpd::Buffer::~Buffer() noexcept {
    Buffer::destroy();
}
