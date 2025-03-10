#pragma once

#include <memory>
#include <memory_resource>

namespace tpd::foundation {

    template <typename T>
    struct Deleter {
        std::pmr::memory_resource* pool;

        void operator()(T* ptr) const noexcept {
            if (ptr) {
                // Only call the destructor if T is not trivially destructible
                if constexpr (!std::is_trivially_destructible_v<T>) {
                    ptr->~T();
                }
                // Deallocate memory
                pool->deallocate(ptr, sizeof(T), alignof(T));
            }
        }
    };

    template <typename T, typename... Args>
    std::unique_ptr<T, Deleter<T>> make_unique(std::pmr::memory_resource* pool, Args&&... args) {
        void* mem = pool->allocate(sizeof(T), alignof(T));  // allocate memory
        T* obj = new (mem) T(std::forward<Args>(args)...);  // placement new
        return std::unique_ptr<T, Deleter<T>>(obj, Deleter<T>{ pool });
    }

    template <typename T, typename... Args>
    std::shared_ptr<T> make_shared(std::pmr::memory_resource* pool, Args&&... args) {
        void* mem = pool->allocate(sizeof(T), alignof(T));
        T* obj = new (mem) T(std::forward<Args>(args)...);
        return std::shared_ptr<T>(obj, Deleter<T>{ pool });
    }

    enum class Alignment : std::size_t {
        None   = 0,
        By_2   = 1 << 1,
        By_4   = 1 << 2,
        By_8   = 1 << 3,
        By_16  = 1 << 4,
        By_32  = 1 << 5,
        By_64  = 1 << 6,
        By_128 = 1 << 7,
        By_256 = 1 << 8,
        By_512 = 1 << 9,
    };

    std::size_t getAlignedSize(std::size_t byteSize, Alignment alignment = Alignment::None);
}
