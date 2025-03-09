#include "torpedo/foundation/AllocationUtils.h"

std::size_t tpd::foundation::getAlignedSize(const std::size_t byteSize, const std::size_t alignment) {
    if (alignment == 0) {
        return byteSize;
    }
    std::size_t multiple = 0;
    while (multiple * alignment < byteSize) ++multiple;
    return  multiple * alignment;
}
