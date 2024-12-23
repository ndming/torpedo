#pragma once

#include "torpedo/foundation/ResourceAllocator.h"

#include <functional>

namespace tpd {
    class Buffer final {
    public:
        class Builder {
        public:
            Builder& bufferCount(uint32_t count);
            Builder& usage(vk::BufferUsageFlags usage);
            Builder& buffer(uint32_t index, std::size_t byteSize, std::size_t alignment = 0);

            [[nodiscard]] std::unique_ptr<Buffer> build(const ResourceAllocator& allocator, ResourceType type);

        private:
            [[nodiscard]] std::unique_ptr<Buffer> buildDedicated (const ResourceAllocator& allocator);
            [[nodiscard]] std::unique_ptr<Buffer> buildPersistent(const ResourceAllocator& allocator);
            [[nodiscard]] vk::BufferCreateInfo populateBufferCreateInfo(vk::BufferUsageFlags internalUsage = {}) const;

            std::vector<std::size_t> _sizes{};
            vk::BufferUsageFlags _usage{};
        };

        Buffer(vk::Buffer buffer, VmaAllocation allocation, std::vector<std::size_t>&& sizes, std::byte* pMappedData = nullptr) noexcept;

        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;

        [[nodiscard]] vk::Buffer getBuffer() const { return _buffer; }
        [[nodiscard]] const std::vector<vk::DeviceSize>& getOffsets() const noexcept { return _offsets; }
        [[nodiscard]] uint32_t getBufferCount() const noexcept { return _offsets.size(); }

        void dispose(const ResourceAllocator& allocator) noexcept;

        void transferBufferData(
            uint32_t bufferIndex,
            const void* data,
            std::size_t dataByteSize,
            const ResourceAllocator& allocator,
            const std::function<void(vk::Buffer, vk::Buffer, vk::BufferCopy)>& onBufferCopy) const;

        void updateBufferData(uint32_t bufferIndex, const void* data, std::size_t dataByteSize) const;

    private:
        vk::Buffer _buffer;
        VmaAllocation _allocation;
        std::byte* _pMappedData;

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
    : _buffer{ buffer }
    , _allocation{ allocation }
    , _pMappedData{ pMappedData }
{
    // Convert to offsets in-place
    _offsets = std::vector(std::move(sizes));
    std::size_t cumulativeSum = 0;
    for (auto& size : _offsets) {
        const auto temp = size;
        size = cumulativeSum;
        cumulativeSum += temp;
    }
}
