#pragma once

#include "torpedo/foundation/ResourceAllocator.h"

#include <vulkan/vulkan.hpp>

#include <functional>
#include <memory>

namespace tpd {
    class Buffer final {
    public:
        class Builder {
        public:
            Builder& bufferCount(uint32_t count);
            Builder& usage(vk::BufferUsageFlags usage);
            Builder& buffer(uint32_t index, std::size_t byteSize, std::size_t alignment = 0);

            [[nodiscard]] std::unique_ptr<Buffer> buildDedicated(const std::unique_ptr<ResourceAllocator>& allocator);

            [[nodiscard]] std::unique_ptr<Buffer> buildPersistent(const std::unique_ptr<ResourceAllocator>& allocator);

        private:
            [[nodiscard]] vk::BufferCreateInfo populateBufferCreateInfo(vk::BufferUsageFlags internalUsage = {}) const;

            std::vector<std::size_t> _sizes{};
            vk::BufferUsageFlags _usage{};
        };

        Buffer(vk::Buffer buffer, VmaAllocation allocation, std::vector<std::size_t>&& sizes, std::byte* pMappedData = nullptr) noexcept;

        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;

        [[nodiscard]] vk::Buffer getBuffer() const { return _buffer; }
        [[nodiscard]] const std::vector<vk::DeviceSize>& getOffsets() const noexcept { return _offsets; }
        [[nodiscard]] uint32_t getBufferCount() const noexcept { return _sizes.size(); }

        void transferBufferData(
            uint32_t bufferIndex,
            const void* data,
            const std::unique_ptr<ResourceAllocator>& allocator,
            const std::function<void(vk::Buffer, vk::Buffer, vk::BufferCopy)>& onBufferCopy) const;

        void updateBufferData(uint32_t bufferIndex, const void* data, std::size_t dataByteSize) const;

        void dispose(const std::unique_ptr<ResourceAllocator>& allocator) noexcept;

    private:
        vk::Buffer _buffer;
        VmaAllocation _allocation;
        std::byte* _pMappedData;

        std::vector<std::size_t> _sizes;
        std::vector<vk::DeviceSize> _offsets;
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::Buffer::Builder& tpd::Buffer::Builder::bufferCount(const uint32_t count) {
    _sizes.resize(count);
    return *this;
}

inline tpd::Buffer::Builder& tpd::Buffer::Builder::usage(const vk::BufferUsageFlags usage) {
    _usage = usage;
    return *this;
}

inline tpd::Buffer::Builder& tpd::Buffer::Builder::buffer(const uint32_t index, const std::size_t byteSize, const std::size_t alignment) {
    std::size_t multiple = 0;
    if (alignment > 0 ) while (multiple * alignment < byteSize) ++multiple;
    _sizes[index] = alignment > 0 ? multiple * alignment : byteSize;
    return *this;
}

inline tpd::Buffer::Buffer(const vk::Buffer buffer, VmaAllocation allocation, std::vector<std::size_t>&& sizes, std::byte* pMappedData) noexcept
: _buffer{ buffer }, _allocation{ allocation }, _pMappedData{ pMappedData }, _sizes{ std::move(sizes) } {
    _offsets = std::vector<vk::DeviceSize>(_sizes.size());
    _offsets[0] = vk::DeviceSize{ 0 };
    for (int i = 0; i < _sizes.size() - 1; ++i) {
        _offsets[i + 1] = vk::DeviceSize{ _offsets[i] + _sizes[i] };
    }
}
