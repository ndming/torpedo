#pragma once

#include "torpedo/foundation/VmaUsage.h"

#include <vulkan/vulkan.hpp>

namespace tpd {
    enum class ResourceType {
        Dedicated,
        Persistent,
    };

    class ResourceAllocator final {
    public:
        class Builder {
        public:
            Builder& flags(const VmaAllocatorCreateFlags flags) {
                _flags = flags;
                return *this;
            }

            Builder& vulkanApiVersion(const uint32_t apiVersion) {
                _apiVersion = apiVersion;
                return *this;
            }

            [[nodiscard]] std::unique_ptr<ResourceAllocator> build(
                vk::Instance instance, vk::PhysicalDevice physicalDevice, vk::Device device) const;

        private:
            VmaAllocatorCreateFlags _flags{};
            uint32_t _apiVersion{ VK_API_VERSION_1_3 };
        };

        explicit ResourceAllocator(VmaAllocator allocator) noexcept : _allocator{ allocator } {
        }

        ~ResourceAllocator() { vmaDestroyAllocator(_allocator); }

        ResourceAllocator(const ResourceAllocator&) = delete;
        ResourceAllocator& operator=(const ResourceAllocator&) = delete;

        vk::Image allocateDedicatedImage(const vk::ImageCreateInfo& imageCreateInfo, VmaAllocation* allocation) const;

        void freeImage(vk::Image image, VmaAllocation allocation) const noexcept;

        vk::Buffer allocateDedicatedBuffer(const vk::BufferCreateInfo& bufferCreateInfo, VmaAllocation* allocation) const;
        vk::Buffer allocatePersistentBuffer(const vk::BufferCreateInfo& bufferCreateInfo, VmaAllocation* allocation, VmaAllocationInfo* allocationInfo) const;
        vk::Buffer allocateStagingBuffer(std::size_t bufferSize, VmaAllocation* allocation) const;

        void mapAndCopyStagingBuffer(std::size_t bufferSize, const void* data, VmaAllocation allocation) const;

        void freeBuffer(vk::Buffer buffer, VmaAllocation allocation) const noexcept;

    private:
        VmaAllocator _allocator;
    };
}
