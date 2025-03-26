#pragma once

#include <torpedo/foundation/SyncResource.h>

#include <type_traits>

namespace tpd {
    template<typename T>
    concept Synchronizable = std::is_base_of_v<SyncResource, T> && std::is_final_v<T>;

    template<Synchronizable T>
    class SyncGroup {
    public:
        [[nodiscard]] std::size_t getMaxSyncSize() const noexcept;
    private:

    };
}  // namespace tpd

template<tpd::Synchronizable T>
std::size_t tpd::SyncGroup<T>::getMaxSyncSize() const noexcept {

}
