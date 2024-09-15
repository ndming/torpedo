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
