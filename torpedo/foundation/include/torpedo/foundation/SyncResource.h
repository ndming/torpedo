#pragma once

#include <vulkan/vulkan.hpp>

namespace tpd {
    class SyncResource {
    public:
        SyncResource(const SyncResource&) = delete;
        SyncResource& operator=(const SyncResource&) = delete;

        SyncResource(SyncResource&& other) noexcept
            : _data{ other._data }, _size{ other._size } {
            other._data = nullptr;
            other._size = 0;
        }

        SyncResource& operator=(SyncResource&& other) noexcept {
            if (this != &other) {
                _data = other._data;
                _size = other._size;
                other._data = nullptr;
                other._size = 0;
            }
            return *this;
        }

        void setSyncData(const void* data, uint32_t byteSize) noexcept;
        [[nodiscard]] bool hasSyncData() const noexcept;

        [[nodiscard]] const std::byte* getSyncData() const noexcept;
        [[nodiscard]] uint32_t getSyncDataSize() const noexcept;

        virtual ~SyncResource() = default;

    protected:
        explicit SyncResource(const void* data = nullptr, const uint32_t byteSize = 0)
            : _data{ static_cast<const std::byte*>(data) }, _size{ byteSize } {
        }

    private:
        const std::byte* _data;
        uint32_t _size;
    };
}  // namespace tpd
