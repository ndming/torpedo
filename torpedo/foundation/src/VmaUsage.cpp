#define VMA_IMPLEMENTATION
#include "torpedo/foundation/VmaUsage.h"

VmaAllocator tpd::vma::Builder::build(
    const vk::Instance instance,
    const vk::PhysicalDevice physicalDevice,
    const vk::Device device) const
{
    auto vulkanFunctions = VmaVulkanFunctions{};
    vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

    auto allocatorCreateInfo = VmaAllocatorCreateInfo{};
    allocatorCreateInfo.flags = _flags;
    allocatorCreateInfo.vulkanApiVersion = _apiVersion;
    allocatorCreateInfo.physicalDevice = physicalDevice;
    allocatorCreateInfo.device = device;
    allocatorCreateInfo.instance = instance;
    allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

    auto allocator = VmaAllocator{};
    if (vmaCreateAllocator(&allocatorCreateInfo, &allocator) != VK_SUCCESS) {
            throw std::runtime_error("vma::Builder - Failed to create a VMA allocator");
    }
    return allocator;
}

vk::Image tpd::vma::allocateDeviceImage(VmaAllocator allocator, const vk::ImageCreateInfo& info, VmaAllocation* allocation) {
    const auto imgCreateInfo = static_cast<VkImageCreateInfo>(info);

    constexpr auto allocInfo = VmaAllocationCreateInfo{
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
        .priority = 1.0f,
    };

    auto image = VkImage{};
    if (vmaCreateImage(allocator, &imgCreateInfo, &allocInfo, &image, allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("VMA - Failed to allocate a dedicated device image");
    }
    return image;
}

void tpd::vma::deallocateImage(VmaAllocator allocator, const vk::Image image, VmaAllocation allocation) noexcept {
    vmaDestroyImage(allocator, image, allocation);
}

vk::Buffer tpd::vma::allocateDeviceBuffer(VmaAllocator allocator, const vk::BufferCreateInfo& info, VmaAllocation* allocation) {
    const auto bufferInfo = static_cast<VkBufferCreateInfo>(info);

    constexpr auto allocInfo = VmaAllocationCreateInfo{
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
        .priority = 1.0f,
    };

    auto buffer = VkBuffer{};
    if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("VMA - Failed to allocate a dedicated device buffer");
    }
    return buffer;
}

vk::Buffer tpd::vma::allocateTwoWayBuffer(
    VmaAllocator allocator,
    const vk::BufferCreateInfo& info,
    VmaAllocation* allocation,
    VmaAllocationInfo* allocationInfo)
{
    const auto bufferInfo = static_cast<VkBufferCreateInfo>(info);

    constexpr auto allocInfo = VmaAllocationCreateInfo{
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    auto buffer = VkBuffer{};
    if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, allocation, allocationInfo) != VK_SUCCESS) {
        throw std::runtime_error("VMA - Failed to allocate a two-way (readback) buffer");
    }
    return buffer;
}

vk::Buffer tpd::vma::allocateMappedBuffer(
    VmaAllocator allocator,
    const vk::BufferCreateInfo& info,
    VmaAllocation* allocation,
    VmaAllocationInfo* allocationInfo)
{
    const auto bufferInfo = static_cast<VkBufferCreateInfo>(info);

    // This approach for creating persistently mapped buffers may not be optimal on systems with unified memory
    // (e.g., AMD APU or Intel integrated graphics, mobile chips)
    constexpr auto allocCreateInfo = VmaAllocationCreateInfo{
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    auto buffer = VkBuffer{};
    if (vmaCreateBuffer(allocator, &bufferInfo, &allocCreateInfo, &buffer, allocation, allocationInfo) != VK_SUCCESS) {
        throw std::runtime_error("VMA - Failed to create a persistently mapped buffer");
    }
    return buffer;
}

std::pair<vk::Buffer, VmaAllocation> tpd::vma::allocateStagingBuffer(VmaAllocator allocator, const vk::DeviceSize size) {
    auto allocation = VmaAllocation{};

    const auto bufferCreateInfo = VkBufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size  = size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    };
    constexpr auto allocCreateInfo = VmaAllocationCreateInfo{
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    auto buffer = VkBuffer{};
    if (vmaCreateBuffer(allocator, &bufferCreateInfo, &allocCreateInfo, &buffer, &allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("VMA - Failed to allocate a staging buffer");
    }
    return std::make_pair(buffer, allocation);
}

void tpd::vma::copyStagingData(VmaAllocator allocator, const void* data, const vk::DeviceSize size, VmaAllocation allocation) {
    void* mappedData;
    vmaMapMemory(allocator, allocation, &mappedData);
    // Memory write by host is guaranteed to be visible
    // prior to the next queue submit
    memcpy(mappedData, data, size);
    vmaUnmapMemory(allocator, allocation);
}

void tpd::vma::deallocateBuffer(VmaAllocator allocator, const vk::Buffer buffer, VmaAllocation allocation) noexcept {
    vmaDestroyBuffer(allocator, buffer, allocation);
}

void tpd::vma::destroy(VmaAllocator allocator) noexcept {
    vmaDestroyAllocator(allocator);
}
