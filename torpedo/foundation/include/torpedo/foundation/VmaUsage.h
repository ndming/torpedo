#pragma once

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace tpd::vma {
    class Builder {
    public:
        Builder& flags(VmaAllocatorCreateFlags flags)  noexcept;
        Builder& vulkanApiVersion(uint32_t apiVersion) noexcept;
        Builder& vulkanApiVersion(uint32_t major, uint32_t minor, uint32_t patch) noexcept;

        [[nodiscard]] VmaAllocator build(vk::Instance instance, vk::PhysicalDevice physicalDevice, vk::Device device) const;

    private:
        VmaAllocatorCreateFlags _flags{};
        uint32_t _apiVersion{ VK_API_VERSION_1_3 };
    };

    [[nodiscard]] vk::Image allocateDeviceImage(
        VmaAllocator allocator, const vk::ImageCreateInfo& imageCreateInfo, VmaAllocation* allocation);

    void deallocateImage(VmaAllocator allocator, vk::Image image, VmaAllocation allocation) noexcept;

    [[nodiscard]] vk::Buffer allocateDeviceBuffer(
        VmaAllocator allocator, const vk::BufferCreateInfo& bufferCreateInfo, VmaAllocation* allocation);
    [[nodiscard]] vk::Buffer allocateTwoWayBuffer(
        VmaAllocator allocator, const vk::BufferCreateInfo& bufferCreateInfo, VmaAllocation* allocation, VmaAllocationInfo* allocationInfo);
    [[nodiscard]] vk::Buffer allocateStagedBuffer(
        VmaAllocator allocator, std::size_t bufferByteSize, VmaAllocation* allocation);
    [[nodiscard]] vk::Buffer allocateMappedBuffer(
        VmaAllocator allocator, const vk::BufferCreateInfo& bufferCreateInfo, VmaAllocation* allocation, VmaAllocationInfo* allocationInfo);

    void deallocateBuffer(VmaAllocator allocator, vk::Buffer buffer, VmaAllocation allocation) noexcept;

    [[nodiscard]] void* mapMemory(VmaAllocator allocator, VmaAllocation allocation);
    void unmapMemory(VmaAllocator allocator, VmaAllocation allocation);

    void destroy(VmaAllocator allocator) noexcept;
} // namespace tpd::vma

inline tpd::vma::Builder& tpd::vma::Builder::flags(const VmaAllocatorCreateFlags flags) noexcept {
    _flags = flags;
    return *this;
}

inline tpd::vma::Builder& tpd::vma::Builder::vulkanApiVersion(const uint32_t apiVersion) noexcept {
    _apiVersion = apiVersion;
    return *this;
}

inline tpd::vma::Builder& tpd::vma::Builder::vulkanApiVersion(
    const uint32_t major,
    const uint32_t minor,
    const uint32_t patch) noexcept
{
    _apiVersion = vk::makeApiVersion(0u, major, minor, patch);
    return *this;
}
