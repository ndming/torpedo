#pragma once

#if !defined(TORPEDO_CONDA_BUILD) && defined(__linux__)
#include <vk_mem_alloc.h>
#else
#include <vma/vk_mem_alloc.h>
#endif

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

    [[nodiscard]] vk::Image allocateDeviceImage(VmaAllocator allocator, const vk::ImageCreateInfo& info, VmaAllocation* allocation);

    void deallocateImage(VmaAllocator allocator, vk::Image image, VmaAllocation allocation) noexcept;

    [[nodiscard]] vk::Buffer allocateDeviceBuffer(VmaAllocator allocator, const vk::BufferCreateInfo& info, VmaAllocation* allocation);
    [[nodiscard]] vk::Buffer allocateTwoWayBuffer(VmaAllocator allocator, const vk::BufferCreateInfo& info, VmaAllocation* allocation, VmaAllocationInfo* allocationInfo);
    [[nodiscard]] vk::Buffer allocateMappedBuffer(VmaAllocator allocator, const vk::BufferCreateInfo& info, VmaAllocation* allocation, VmaAllocationInfo* allocationInfo);

    [[nodiscard]] std::pair<vk::Buffer, VmaAllocation> allocateStagingBuffer(VmaAllocator allocator, vk::DeviceSize size);
    void copyStagingData(VmaAllocator allocator, const void* data, vk::DeviceSize size, VmaAllocation allocation);

    void deallocateBuffer(VmaAllocator allocator, vk::Buffer buffer, VmaAllocation allocation) noexcept;

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

namespace tpd {
    template<typename T> requires(std::is_same_v<T, vk::Buffer> || std::is_same_v<T, vk::Image>)
    class OpaqueResource {
    public:
        OpaqueResource() noexcept = default;
        OpaqueResource(T resource, VmaAllocation allocation) noexcept;

        OpaqueResource(OpaqueResource&& other) noexcept;
        OpaqueResource& operator=(OpaqueResource&& other) noexcept;

        [[nodiscard]] operator T() const noexcept { return _resource; } // NOLINT(*-explicit-constructor)
        [[nodiscard]] bool valid() const noexcept { return _allocation != nullptr; }
        [[nodiscard]] std::string toString() const noexcept;
        [[nodiscard]] VmaAllocation getAllocation() const noexcept { return _allocation; }

        void destroy(VmaAllocator allocator) noexcept;

    protected:
        T _resource{};

    private:
        VmaAllocation _allocation{ nullptr };
    };

    struct SyncPoint {
        vk::PipelineStageFlags2 stage{};
        vk::AccessFlags2 access{};
    };
} // namespace tpd

template <typename T> requires(std::is_same_v<T, vk::Buffer> || std::is_same_v<T, vk::Image>)
tpd::OpaqueResource<T>::OpaqueResource(const T resource, VmaAllocation allocation) noexcept
    : _resource{ resource }
    , _allocation{ allocation }
{}

template<typename T> requires(std::is_same_v<T, vk::Buffer> || std::is_same_v<T, vk::Image>)
tpd::OpaqueResource<T>::OpaqueResource(OpaqueResource&& other) noexcept
    : _resource{ other._resource }
    , _allocation{ other._allocation }
{
    other._resource = nullptr;
    other._allocation = nullptr;
}

template<typename T> requires (std::is_same_v<T, vk::Buffer> || std::is_same_v<T, vk::Image>)
tpd::OpaqueResource<T>& tpd::OpaqueResource<T>::operator=(OpaqueResource&& other) noexcept {
    if (this == &other || valid()) {
        return *this;
    }

    _resource = other._resource;
    _allocation = other._allocation;

    other._resource = nullptr;
    other._allocation = nullptr;
    return *this;
}

template<typename T> requires (std::is_same_v<T, vk::Buffer> || std::is_same_v<T, vk::Image>)
std::string tpd::OpaqueResource<T>::toString() const noexcept {
    char buffer[20];
    std::snprintf(buffer, sizeof(buffer), "%p", static_cast<const void*>(_resource));
    return buffer;
}

template<typename T> requires(std::is_same_v<T, vk::Buffer> || std::is_same_v<T, vk::Image>)
void tpd::OpaqueResource<T>::destroy(VmaAllocator allocator) noexcept {
    if (!valid())
        return;

    if constexpr (std::is_same_v<T, vk::Buffer>) {
        vma::deallocateBuffer(allocator, _resource, _allocation);
    } else if constexpr (std::is_same_v<T, vk::Image>) {
        vma::deallocateImage(allocator, _resource, _allocation);
    }

    _resource = nullptr;
    _allocation = nullptr;
}
