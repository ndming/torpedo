#pragma once

#include <cstddef>
#include <cstdint>

namespace tpd {
    class SyncResource {
    public:
        void setSyncData(const void* data, uint32_t byteSize) noexcept;

        [[nodiscard]] const std::byte* getSyncData() const noexcept;
        [[nodiscard]] uint32_t getSyncDataSize() const noexcept;

        [[nodiscard]] bool hasSyncData() const noexcept;

    private:
        const std::byte* _data{ nullptr };
        uint32_t _size{ 0 };
    };
}
