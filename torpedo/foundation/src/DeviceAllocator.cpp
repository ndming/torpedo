#include "torpedo/foundation/DeviceAllocator.h"

tpd::DeviceAllocator tpd::DeviceAllocator::Builder::build(
    const vk::Instance instance,
    const vk::PhysicalDevice physicalDevice,
    const vk::Device device) const
{
    const auto allocator = createAllocator(instance, physicalDevice, device);
    return DeviceAllocator{ allocator };
}

std::unique_ptr<tpd::DeviceAllocator, tpd::Deleter<tpd::DeviceAllocator>> tpd::DeviceAllocator::Builder::build(
    const vk::Instance instance,
    const vk::PhysicalDevice physicalDevice,
    const vk::Device device,
    std::pmr::memory_resource* const pool) const
{
    const auto allocator = createAllocator(instance, physicalDevice, device);
    return foundation::make_unique<DeviceAllocator>(pool, allocator);
}

VmaAllocator tpd::DeviceAllocator::Builder::createAllocator(
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
        throw std::runtime_error("DeviceAllocator::Builder - Failed to create a VMA allocator");
    }
    return allocator;
}

vk::Image tpd::DeviceAllocator::allocateDeviceImage(
    const vk::ImageCreateInfo& imageCreateInfo,
    VmaAllocation* allocation) const
{
    const auto imgCreateInfo = static_cast<VkImageCreateInfo>(imageCreateInfo);

    constexpr auto allocInfo = VmaAllocationCreateInfo{
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
        .priority = 1.0f,
    };

    auto image = VkImage{};
    if (vmaCreateImage(_vmaAllocator, &imgCreateInfo, &allocInfo, &image, allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("DeviceAllocator - Failed to allocate a dedicated device image");
    }
    return image;
}

void tpd::DeviceAllocator::deallocateImage(const vk::Image image, VmaAllocation allocation) const noexcept {
    vmaDestroyImage(_vmaAllocator, image, allocation);
}

vk::Buffer tpd::DeviceAllocator::allocateDeviceBuffer(
    const vk::BufferCreateInfo& bufferCreateInfo,
    VmaAllocation* allocation) const
{
    const auto bufferInfo = static_cast<VkBufferCreateInfo>(bufferCreateInfo);

    constexpr auto allocInfo = VmaAllocationCreateInfo{
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
        .priority = 1.0f,
    };

    auto buffer = VkBuffer{};
    if (vmaCreateBuffer(_vmaAllocator, &bufferInfo, &allocInfo, &buffer, allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("DeviceAllocator - Failed to allocate a dedicated device buffer");
    }
    return buffer;
}

vk::Buffer tpd::DeviceAllocator::allocateTwoWayBuffer(
    const vk::BufferCreateInfo& bufferCreateInfo,
    VmaAllocation* allocation,
    VmaAllocationInfo* allocationInfo) const
{
    const auto bufferInfo = static_cast<VkBufferCreateInfo>(bufferCreateInfo);

    constexpr auto allocInfo = VmaAllocationCreateInfo{
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    auto buffer = VkBuffer{};
    if (vmaCreateBuffer(_vmaAllocator, &bufferInfo, &allocInfo, &buffer, allocation, allocationInfo) != VK_SUCCESS) {
        throw std::runtime_error("DeviceAllocator - Failed to allocate a two-way buffer");
    }
    return buffer;
}

vk::Buffer tpd::DeviceAllocator::allocateStagedBuffer(const std::size_t bufferByteSize, VmaAllocation* allocation) const {
    const auto bufferCreateInfo = VkBufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size  = bufferByteSize,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    };
    constexpr auto allocCreateInfo = VmaAllocationCreateInfo{
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    auto buffer = VkBuffer{};
    if (vmaCreateBuffer(_vmaAllocator, &bufferCreateInfo, &allocCreateInfo, &buffer, allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("DeviceAllocator - Failed to allocate a staging buffer");
    }
    return buffer;
}

vk::Buffer tpd::DeviceAllocator::allocateMappedBuffer(
    const vk::BufferCreateInfo& bufferCreateInfo,
    VmaAllocation* allocation,
    VmaAllocationInfo* allocationInfo) const
{
    const auto bufferInfo = static_cast<VkBufferCreateInfo>(bufferCreateInfo);

    // This approach for creating persistently mapped buffers may not be optimal on systems with unified memory
    // (e.g., AMD APU or Intel integrated graphics, mobile chips)
    constexpr auto allocCreateInfo = VmaAllocationCreateInfo{
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    auto buffer = VkBuffer{};
    if (vmaCreateBuffer(_vmaAllocator, &bufferInfo, &allocCreateInfo, &buffer, allocation, allocationInfo) != VK_SUCCESS) {
        throw std::runtime_error("DeviceAllocator - Failed to create a persistently mapped buffer");
    }
    return buffer;
}

void tpd::DeviceAllocator::deallocateBuffer(const vk::Buffer buffer, VmaAllocation allocation) const noexcept {
    vmaDestroyBuffer(_vmaAllocator, buffer, allocation);
}

void* tpd::DeviceAllocator::mapMemory(VmaAllocation allocation) const {
    void* mappedData;
    vmaMapMemory(_vmaAllocator, allocation, &mappedData);
    return mappedData;
}

void tpd::DeviceAllocator::unmapMemory(VmaAllocation allocation) const {
    vmaUnmapMemory(_vmaAllocator, allocation);
}
