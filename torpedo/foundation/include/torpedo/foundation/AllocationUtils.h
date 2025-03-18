#pragma once

#include <memory>
#include <memory_resource>
#include <type_traits>

namespace tpd {
    /**
     * @enum Alignment
     *
     * @brief Enum class representing alignment values in bytes, all powers of 2.
     *
     * This enum provides type-safe alignment values in bytes for memory allocations, where each entry
     * corresponds to a power of 2 up to 512 bytes.
     */
    enum class Alignment : std::size_t {
        /**
         * @brief No alignment requirement.
         */
        None = 0,

        /**
         * @brief Align by 2 bytes.
         */
        By_2 = 1 << 1,

        /**
         * @brief Align by 4 bytes.
         */
        By_4 = 1 << 2,

        /**
         * @brief Align by 8 bytes.
         */
        By_8 = 1 << 3,

        /**
         * @brief Align by 16 bytes.
         */
        By_16 = 1 << 4,

        /**
         * @brief Align by 32 bytes.
         */
        By_32 = 1 << 5,

        /**
         * @brief Align by 64 bytes.
         */
        By_64 = 1 << 6,

        /**
         * @brief Align by 128 bytes.
         */
        By_128 = 1 << 7,

        /**
         * @brief Align by 256 bytes.
         */
        By_256 = 1 << 8,

        /**
         * @brief Align by 512 bytes.
         */
        By_512 = 1 << 9,
    };

    template <typename T>
    struct Deleter {
        std::pmr::memory_resource* pool;
        constexpr void operator()(T* ptr) const noexcept {
            static_assert(0 < sizeof(T), "Deleter - Can't delete an incomplete type");
            if constexpr (!std::is_trivially_destructible_v<T>) {
                ptr->~T();
            }
            pool->deallocate(ptr, sizeof(T), alignof(T));
        }
    };
}

namespace tpd::foundation {
    template <typename T, typename... Args> requires (!std::is_array_v<T>)
    std::unique_ptr<T, Deleter<T>> make_unique(std::pmr::memory_resource* pool, Args&&... args) {
        void* mem = pool->allocate(sizeof(T), alignof(T));  // allocate memory
        T* obj = new (mem) T(std::forward<Args>(args)...);  // placement new
        return std::unique_ptr<T, Deleter<T>>(obj, Deleter<T>{ pool });
    }

    template <typename T, std::enable_if_t<std::is_array_v<T> && std::extent_v<T> == 0, int> = 0>
    void make_unique(std::pmr::memory_resource*, std::size_t) = delete;

    template <typename T, typename... Args, std::enable_if_t<std::extent_v<T> != 0, int> = 0>
    void make_unique(std::pmr::memory_resource*, Args&&...) = delete;

    /**
     * @brief Calculate the aligned size of a given byte size according to the specified alignment.
     *
     * This function calculates the smallest size that is greater than or equal to the given byteSize,
     * ensuring that the size is aligned to the specified alignment boundary. If no alignment is provided,
     * the function returns the byteSize as is.
     *
     * @param byteSize  The size in bytes to be aligned.
     * @param alignment The alignment boundary, default to Alignment::None
     * @return The aligned size that is greater than or equal to the byteSize and aligned to the alignment boundary.
     */
    std::size_t getAlignedSize(std::size_t byteSize, Alignment alignment = Alignment::None);
}
