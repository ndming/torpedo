#include "torpedo/foundation/AllocationUtils.h"

std::size_t tpd::foundation::getAlignedSize(const std::size_t byteSize, const std::size_t alignment) {
    if (alignment == 0) {
        return byteSize;
    }
    // Return the least multiple of alignment greater than or equal byteSize
    return byteSize + alignment - 1 & ~(alignment - 1);
}
