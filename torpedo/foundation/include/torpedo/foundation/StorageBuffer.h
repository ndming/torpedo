#pragma once

#include "torpedo/foundation/Buffer.h"
#include "torpedo/foundation/SyncResource.h"

namespace tpd {
    class StorageBuffer final : public Buffer, public SyncResource {
    public:
        class Builder {
        public:
            Builder& usage(vk::BufferUsageFlags usage) noexcept;
            Builder& alloc(std::size_t byteSize, Alignment alignment = Alignment::None) noexcept;

            Builder& syncData(const void* data, uint32_t byteSize = 0) noexcept;
            Builder& dstPoint(vk::PipelineStageFlags2 stage, vk::AccessFlags2 access) noexcept;

            [[nodiscard]] StorageBuffer build(const DeviceAllocator& allocator) const;

            [[nodiscard]] std::unique_ptr<StorageBuffer, Deleter<StorageBuffer>> build(
                const DeviceAllocator& allocator, std::pmr::memory_resource* pool) const;

        private:
            [[nodiscard]] vk::Buffer creatBuffer(const DeviceAllocator& allocator, VmaAllocation* allocation) const;

            vk::BufferUsageFlags _usage{};
            std::size_t _allocSize{};

            const void* _data{ nullptr };
            uint32_t _dataSize{ 0 };

            using StageMask = vk::PipelineStageFlagBits2;
            SyncPoint _dstPoint{
                StageMask::eVertexShader | StageMask::eFragmentShader | StageMask::eComputeShader,
                vk::AccessFlagBits2::eShaderStorageRead | vk::AccessFlagBits2::eShaderStorageWrite,
            };
        };

        StorageBuffer(
            SyncPoint dstSyncPoint, vk::Buffer buffer, VmaAllocation allocation, const DeviceAllocator& allocator,
            const void* data = nullptr, uint32_t dataByteSize = 0);

        void recordBufferTransfer(vk::CommandBuffer cmd, vk::Buffer stagingBuffer) const noexcept;
        void recordDstSync(vk::CommandBuffer cmd) const noexcept;

        void recordOwnershipRelease(vk::CommandBuffer cmd, uint32_t srcFamilyIndex, uint32_t dstFamilyIndex) const noexcept;
        void recordOwnershipAcquire(vk::CommandBuffer cmd, uint32_t srcFamilyIndex, uint32_t dstFamilyIndex) const noexcept;

    private:
        SyncPoint _dstSyncPoint;
    };
}  // namespace tpd

inline tpd::StorageBuffer::Builder& tpd::StorageBuffer::Builder::usage(const vk::BufferUsageFlags usage) noexcept {
    _usage = usage;
    return *this;
}

inline tpd::StorageBuffer::Builder& tpd::StorageBuffer::Builder::alloc(const std::size_t byteSize, const Alignment alignment) noexcept {
    _allocSize = foundation::getAlignedSize(byteSize, alignment);
    return *this;
}

inline tpd::StorageBuffer::Builder& tpd::StorageBuffer::Builder::syncData(const void* data, const uint32_t byteSize) noexcept {
    _data = data;
    _dataSize = byteSize;
    return *this;
}

inline tpd::StorageBuffer::Builder& tpd::StorageBuffer::Builder::dstPoint(
    const vk::PipelineStageFlags2 stage,
    const vk::AccessFlags2 access) noexcept
{
    _dstPoint.stage = stage;
    _dstPoint.access = access;
    return *this;
}

inline tpd::StorageBuffer::StorageBuffer(
    const SyncPoint dstSyncPoint,
    const vk::Buffer buffer,
    VmaAllocation allocation,
    const DeviceAllocator& allocator,
    const void* data,
    const uint32_t dataByteSize)
    : Buffer{ buffer, allocation, allocator }
    , SyncResource{ data, dataByteSize }
    , _dstSyncPoint{ dstSyncPoint }
{}
