#include "torpedo/foundation/ReadbackBuffer.h"

tpd::ReadbackBuffer tpd::ReadbackBuffer::Builder::build(const DeviceAllocator& allocator) const {
    auto allocation = VmaAllocation{};         // allocation is a pointer
    auto allocationInfo = VmaAllocationInfo{}; // allocation info is a struct

    const auto bufferCreateInfo = vk::BufferCreateInfo{ {}, _allocSize, _usage };
    const auto buffer = allocator.allocateTwoWayBuffer(bufferCreateInfo, &allocation, &allocationInfo);

    return ReadbackBuffer{ allocationInfo.pMappedData, _allocSize, buffer, allocation, allocator };
}

std::unique_ptr<tpd::ReadbackBuffer, tpd::Deleter<tpd::ReadbackBuffer>> tpd::ReadbackBuffer::Builder::build(
    const DeviceAllocator& allocator,
    std::pmr::memory_resource* pool) const
{
    auto allocation = VmaAllocation{};         // allocation is a pointer
    auto allocationInfo = VmaAllocationInfo{}; // allocation info is a struct

    const auto bufferCreateInfo = vk::BufferCreateInfo{ {}, _allocSize, _usage };
    const auto buffer = allocator.allocateTwoWayBuffer(bufferCreateInfo, &allocation, &allocationInfo);

    return foundation::make_unique<ReadbackBuffer>(pool, allocationInfo.pMappedData, _allocSize, buffer, allocation, allocator);
}

float tpd::ReadbackBuffer::readFloat() const noexcept {
    return *static_cast<const float*>(_pMappedData);
}

std::vector<float> tpd::ReadbackBuffer::readFloatArray(const uint32_t count) const noexcept {
    auto data = std::vector<float>(count);
    memcpy(data.data(), _pMappedData, count * sizeof(float));
    return data;
}

uint32_t tpd::ReadbackBuffer::readUint32() const noexcept {
    return *static_cast<const uint32_t*>(_pMappedData);
}

std::vector<uint32_t> tpd::ReadbackBuffer::readUint32Array(const uint32_t count) const noexcept {
    auto data = std::vector<uint32_t>(count);
    memcpy(data.data(), _pMappedData, count * sizeof(uint32_t));
    return data;
}