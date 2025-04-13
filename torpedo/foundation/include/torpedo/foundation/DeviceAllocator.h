#pragma once

#include "torpedo/foundation/VmaUsage.h"

#include <vulkan/vulkan.hpp>

namespace tpd {
    class DeviceAllocator final {
    public:
        class Builder {
        public:
            Builder& flags(VmaAllocatorCreateFlags flags)  noexcept;
            Builder& vulkanApiVersion(uint32_t apiVersion) noexcept;
            Builder& vulkanApiVersion(uint32_t major, uint32_t minor, uint32_t patch) noexcept;

            [[nodiscard]] DeviceAllocator build(vk::Instance instance, vk::PhysicalDevice physicalDevice, vk::Device device) const;

        private:
            VmaAllocatorCreateFlags _flags{};
            uint32_t _apiVersion{ VK_API_VERSION_1_3 };
        };

        explicit DeviceAllocator(VmaAllocator vmaAllocator = nullptr);

        DeviceAllocator(const DeviceAllocator&) = delete;
        DeviceAllocator& operator=(const DeviceAllocator&) = delete;

        DeviceAllocator& operator=(DeviceAllocator&& other) noexcept {
            if (this != &other) [[likely]] {
                _vmaAllocator = other._vmaAllocator;
                other._vmaAllocator = nullptr;
            }
            return *this;
        }

        [[nodiscard]] vk::Image allocateDeviceImage(const vk::ImageCreateInfo& imageCreateInfo, VmaAllocation* allocation) const;

        void deallocateImage(vk::Image image, VmaAllocation allocation) const noexcept;

        [[nodiscard]] vk::Buffer allocateDeviceBuffer(const vk::BufferCreateInfo& bufferCreateInfo, VmaAllocation* allocation) const;
        [[nodiscard]] vk::Buffer allocateTwoWayBuffer(const vk::BufferCreateInfo& bufferCreateInfo, VmaAllocation* allocation, VmaAllocationInfo* allocationInfo) const;
        [[nodiscard]] vk::Buffer allocateStagedBuffer(std::size_t bufferByteSize, VmaAllocation* allocation) const;
        [[nodiscard]] vk::Buffer allocateMappedBuffer(const vk::BufferCreateInfo& bufferCreateInfo, VmaAllocation* allocation, VmaAllocationInfo* allocationInfo) const;

        void deallocateBuffer(vk::Buffer buffer, VmaAllocation allocation) const noexcept;

        [[nodiscard]] void* mapMemory(VmaAllocation allocation) const;
        void unmapMemory(VmaAllocation allocation) const;

        void destroy() noexcept;
        ~DeviceAllocator() noexcept;

    private:
        VmaAllocator _vmaAllocator;
    };

    class Destroyable {
    public:
        Destroyable(VmaAllocation allocation, const DeviceAllocator& allocator)
            : _allocation{ allocation }, _allocator{ allocator } {}

        Destroyable(const Destroyable&) = delete;
        Destroyable& operator=(const Destroyable&) = delete;

        Destroyable(Destroyable&& other) noexcept
            : _allocation{ other._allocation }, _allocator{ other._allocator } {
            other._allocation = nullptr;
        }

        Destroyable& operator=(Destroyable&& other) noexcept {
            if (this != &other) [[likely]] {
                Destroyable::destroy();
                _allocation = other._allocation;
                other._allocation = nullptr;
            }
            return *this;
        }

        virtual void destroy() noexcept {
            _allocation = nullptr;
        }

        virtual ~Destroyable() noexcept {
            Destroyable::destroy();
        }

    protected:
        VmaAllocation _allocation;
        const DeviceAllocator& _allocator;
    };
} // namespace tpd

inline tpd::DeviceAllocator::Builder& tpd::DeviceAllocator::Builder::flags(const VmaAllocatorCreateFlags flags) noexcept {
    _flags = flags;
    return *this;
}

inline tpd::DeviceAllocator::Builder& tpd::DeviceAllocator::Builder::vulkanApiVersion(const uint32_t apiVersion) noexcept {
    _apiVersion = apiVersion;
    return *this;
}

inline tpd::DeviceAllocator::Builder& tpd::DeviceAllocator::Builder::vulkanApiVersion(
    const uint32_t major,
    const uint32_t minor,
    const uint32_t patch) noexcept
{
    _apiVersion = vk::makeApiVersion(0u, major, minor, patch);
    return *this;
}

inline tpd::DeviceAllocator::DeviceAllocator(VmaAllocator vmaAllocator) {
    _vmaAllocator = vmaAllocator;
}


inline void tpd::DeviceAllocator::destroy() noexcept {
    if (_vmaAllocator) {
        vmaDestroyAllocator(_vmaAllocator);
        _vmaAllocator = nullptr;
    }
}

inline tpd::DeviceAllocator::~DeviceAllocator() noexcept {
    destroy();
}
