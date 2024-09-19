#include "torpedo/foundation/ResourceAllocator.h"

std::unique_ptr<tpd::ResourceAllocator> tpd::ResourceAllocator::Builder::build(
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
    vmaCreateAllocator(&allocatorCreateInfo, &allocator);

    return std::make_unique<ResourceAllocator>(allocator);
}

vk::Image tpd::ResourceAllocator::allocateDedicatedImage(const vk::ImageCreateInfo& imageCreateInfo, VmaAllocation* allocation) const {
    auto allocInfo = VmaAllocationCreateInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    // This flag is preferable for resources that are large and get destroyed or recreated with different sizes
    allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    // When VK_EXT_memory_priority extension is enabled, it is also worth setting high priority to such allocation
    // to decrease chances to be evicted to system memory by the operating system
    allocInfo.priority = 1.0f;

    const auto imgCreateInfo = static_cast<VkImageCreateInfo>(imageCreateInfo);
    auto image = VkImage{};

    if (vmaCreateImage(_allocator, &imgCreateInfo, &allocInfo, &image, allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate a dedicated image");
    }

    return image;
}

void tpd::ResourceAllocator::destroyImage(const vk::Image image, VmaAllocation allocation) const noexcept {
    vmaDestroyImage(_allocator, image, allocation);
}

vk::Buffer tpd::ResourceAllocator::allocateDedicatedBuffer(const vk::BufferCreateInfo& bufferCreateInfo, VmaAllocation* allocation) const {
    const auto bufferInfo = static_cast<VkBufferCreateInfo>(bufferCreateInfo);

    auto allocInfo = VmaAllocationCreateInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    allocInfo.priority = 1.0f;

    auto buffer = VkBuffer{};
    if (vmaCreateBuffer(_allocator, &bufferInfo, &allocInfo, &buffer, allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate a dedicated buffer");
    }

    return buffer;
}

vk::Buffer tpd::ResourceAllocator::allocatePersistentBuffer(
    const vk::BufferCreateInfo& bufferCreateInfo,
    VmaAllocation* allocation,
    VmaAllocationInfo* allocationInfo) const
{
    const auto bufferInfo = static_cast<VkBufferCreateInfo>(bufferCreateInfo);

    // This approach for creating persistently mapped buffers may not be optimal on systems with unified memory
    // (e.g., AMD APU or Intel integrated graphics, mobile chips)
    auto allocCreateInfo = VmaAllocationCreateInfo{};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocCreateInfo.flags =  VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    auto buffer = VkBuffer{};
    if (vmaCreateBuffer(_allocator, &bufferInfo, &allocCreateInfo, &buffer, allocation, allocationInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create a persistently mapped buffer");
    }

    return buffer;
}

vk::Buffer tpd::ResourceAllocator::allocateStagingBuffer(const std::size_t bufferSize, VmaAllocation* allocation) const {
    auto bufferCreateInfo = VkBufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferCreateInfo.size = bufferSize;

    auto allocCreateInfo = VmaAllocationCreateInfo{};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    auto buffer = VkBuffer{};
    if (vmaCreateBuffer(_allocator, &bufferCreateInfo, &allocCreateInfo, &buffer, allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate a staging buffer");
    }

    return buffer;
}

void tpd::ResourceAllocator::mapAndCopyStagingBuffer(const std::size_t bufferSize, const void* const data, VmaAllocation allocation) const {
    void* mappedData;
    vmaMapMemory(_allocator, allocation, &mappedData);
    memcpy(mappedData, data, bufferSize);
    vmaUnmapMemory(_allocator, allocation);
}

void tpd::ResourceAllocator::destroyBuffer(const vk::Buffer buffer, VmaAllocation allocation) const noexcept {
    vmaDestroyBuffer(_allocator, buffer, allocation);
}
