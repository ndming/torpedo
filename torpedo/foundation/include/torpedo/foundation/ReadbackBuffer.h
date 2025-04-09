#pragma once

#include "torpedo/foundation/Buffer.h"

namespace tpd {
    class ReadbackBuffer final : public Buffer {
    public:
        class Builder {
        public:
            Builder& usage(vk::BufferUsageFlags usage) noexcept;
            Builder& alloc(std::size_t byteSize, std::size_t alignment = 0) noexcept;

            [[nodiscard]] ReadbackBuffer build(const DeviceAllocator& allocator) const;

            [[nodiscard]] std::unique_ptr<ReadbackBuffer, Deleter<ReadbackBuffer>> build(
                const DeviceAllocator& allocator, std::pmr::memory_resource* pool) const;

        private:
            vk::BufferUsageFlags _usage{};
            std::size_t _allocSize{};
        };

        ReadbackBuffer(
            void* pMappedData, std::size_t allocSize, vk::Buffer buffer,
            VmaAllocation allocation, const DeviceAllocator& allocator);

        [[nodiscard]] void* readData() const noexcept;

        [[nodiscard]] float readFloat() const noexcept;
        [[nodiscard]] std::vector<float> readFloatArray(uint32_t count) const noexcept;

        [[nodiscard]] uint32_t readUint32() const noexcept;
        [[nodiscard]] std::vector<uint32_t> readUint32Array(uint32_t count) const noexcept;
        
        void destroy() noexcept override;
        ~ReadbackBuffer() noexcept override;

    private:
        void* _pMappedData;
    };
} // namespace tpd

inline tpd::ReadbackBuffer::Builder& tpd::ReadbackBuffer::Builder::usage(const vk::BufferUsageFlags usage) noexcept {
    _usage = usage;
    return *this;
}

inline tpd::ReadbackBuffer::Builder& tpd::ReadbackBuffer::Builder::alloc(const std::size_t byteSize, const std::size_t alignment) noexcept {
    _allocSize  = foundation::getAlignedSize(byteSize, alignment);
    return *this;
}

inline tpd::ReadbackBuffer::ReadbackBuffer(
    void* pMappedData,
    const std::size_t allocSize,
    const vk::Buffer buffer,
    VmaAllocation allocation,
    const DeviceAllocator& allocator)
    : Buffer{ allocSize, buffer, allocation, allocator}
    , _pMappedData(pMappedData)
{}

inline void* tpd::ReadbackBuffer::readData() const noexcept {
    return _pMappedData;
}

inline void tpd::ReadbackBuffer::destroy() noexcept {
    _pMappedData = nullptr;
    Buffer::destroy();
}

inline tpd::ReadbackBuffer::~ReadbackBuffer() noexcept {
    destroy();
}
