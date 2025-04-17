#include "torpedo/foundation/Sync.h"

void tpd::SyncResource::setSyncData(const void* const data, const uint32_t byteSize) noexcept {
    _data = static_cast<const std::byte*>(data);
    _size = byteSize;
}

bool tpd::SyncResource::hasSyncData() const noexcept {
    return _data != nullptr && _size > 0;
}

const std::byte* tpd::SyncResource::getSyncData() const noexcept {
    return _data;
}

uint32_t tpd::SyncResource::getSyncDataSize() const noexcept {
    return _size;
}
