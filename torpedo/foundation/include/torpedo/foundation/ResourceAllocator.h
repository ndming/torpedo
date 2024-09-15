#pragma once

#include <torpedo/foundation/VmaUsage.h>

#include <vulkan/vulkan.hpp>

#include <memory>

namespace tpd {
    class ResourceAllocator {
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

        vk::Image allocateDedicatedImage(const vk::ImageCreateInfo& imageCreateInfo, VmaAllocation* allocation) const;

        void destroyImage(vk::Image image, VmaAllocation allocation) const noexcept;

        explicit ResourceAllocator(VmaAllocator allocator) : _allocator{ allocator } {
        }

        ~ResourceAllocator() {
            vmaDestroyAllocator(_allocator);
        }

        ResourceAllocator(const ResourceAllocator&) = delete;
        ResourceAllocator& operator=(const ResourceAllocator&) = delete;

    private:
        VmaAllocator _allocator;
    };
}
