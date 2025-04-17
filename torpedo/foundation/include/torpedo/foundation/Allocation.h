#pragma once

#include <memory>
#include <memory_resource>
#include <type_traits>

namespace tpd {
    template <typename T>
    struct Deleter {
        std::pmr::memory_resource* pool;
        constexpr void operator()(T* ptr) const noexcept {
            // Should we check if T is a complete type before deleting it?
            // static_assert(sizeof(T) > 0, "Deleter - Can't delete an incomplete type");
            if constexpr (!std::is_trivially_destructible_v<T>) {
                ptr->~T();
            }
            pool->deallocate(ptr, sizeof(T), alignof(T));
        }
    };
} // namespace tpd

namespace tpd::alloc {
    /**
     * Returns the smallest size in bytes that is greater than or equal to `size` and is divisible by `alignment`.
     *
     * @param byteSize The minimum size in bytes to align up.
     * @param alignment The alignment boundary, which must be a power of two. When set to 0, `byteSize` is returned as is.
     */
    constexpr std::size_t alignUp(const std::size_t size, const std::size_t alignment = 0) {
        if (alignment == 0) {
            return size;
        }
        // Return the least multiple of alignment greater than or equal byteSize
        return size + alignment - 1 & ~(alignment - 1);
    }

    template <typename T, typename... Args> requires (!std::is_array_v<T>)
    std::unique_ptr<T, Deleter<T>> make_unique(std::pmr::memory_resource* pool, Args&&... args) {
        void* mem = pool->allocate(sizeof(T), alignof(T)); // allocate memory
        T* obj = new (mem) T(std::forward<Args>(args)...); // placement new
        return std::unique_ptr<T, Deleter<T>>(obj, Deleter<T>{ pool });
    }

    template <typename T, std::enable_if_t<std::is_array_v<T> && std::extent_v<T> == 0, int> = 0>
    void make_unique(std::pmr::memory_resource*, std::size_t) = delete;

    template <typename T, typename... Args, std::enable_if_t<std::extent_v<T> != 0, int> = 0>
    void make_unique(std::pmr::memory_resource*, Args&&...) = delete;
} // namespace tpd::alloc
