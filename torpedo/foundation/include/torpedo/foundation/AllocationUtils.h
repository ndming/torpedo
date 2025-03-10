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
