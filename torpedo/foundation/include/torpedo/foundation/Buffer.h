#pragma once

#include "torpedo/foundation/Allocation.h"
#include "torpedo/foundation/VmaUsage.h"

namespace tpd {
    class Buffer : public OpaqueResource<vk::Buffer> {
    public:
        Buffer() noexcept = default;
        Buffer(vk::Buffer buffer, VmaAllocation allocation);

        void recordOwnershipRelease(vk::CommandBuffer cmd, uint32_t srcFamily, uint32_t dstFamily, SyncPoint srcSync) const noexcept;
        void recordOwnershipAcquire(vk::CommandBuffer cmd, uint32_t srcFamily, uint32_t dstFamily, SyncPoint dstSync) const noexcept;

        template<typename B, typename T>
        class Builder {
        public:
            B& usage(const vk::BufferUsageFlags usage) noexcept {
                _usage = usage;
                return *static_cast<B*>(this);
            }

            B& alloc(const vk::DeviceSize size, const vk::DeviceSize alignment = 0) noexcept {
                _allocSize = alloc::alignUp(size, alignment);
                return *static_cast<B*>(this);
            }

            [[nodiscard]] virtual T build(VmaAllocator allocator) const = 0;
            virtual ~Builder() = default;

        protected:
            vk::BufferUsageFlags _usage{};
            vk::DeviceSize _allocSize{};
        };
    };
} // namespace tpd

inline tpd::Buffer::Buffer(const vk::Buffer buffer, VmaAllocation allocation)
    : OpaqueResource{ buffer, allocation } {}