#include "torpedo/foundation/Image.h"
#include "torpedo/foundation/ImageUtils.h"

void tpd::Image::recordImageTransition(const vk::CommandBuffer cmd, const vk::ImageLayout newLayout) {
    const auto oldLayout = _layout;
    foundation::recordLayoutTransition(cmd, _image, getAspectMask(), getMipLevelCount(), oldLayout, newLayout);
    _layout = newLayout;
}
