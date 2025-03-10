#include "torpedo/foundation/AllocationUtils.h"

std::size_t tpd::foundation::getAlignedSize(const std::size_t byteSize, const Alignment alignment) {
    if (alignment == Alignment::None) {
        return byteSize;
    }
    // Return the least multiple of alignment greater than or equal byteSize
    const auto alignmentValue = static_cast<std::size_t>(alignment);
    return byteSize + alignmentValue - 1 & ~(alignmentValue - 1);
}
