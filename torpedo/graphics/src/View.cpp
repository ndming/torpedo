#include "torpedo/graphics/View.h"

void tpd::View::setSize(const uint32_t width, const uint32_t height) {
    setViewport(static_cast<float>(width), static_cast<float>(height));
    setScissor(width, height);
}
