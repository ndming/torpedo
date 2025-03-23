#pragma once

#include <torpedo/rendering/Engine.h>

namespace tpd {
    class GaussianEngine final : public Engine {
    public:
        [[nodiscard]] DrawPackage draw(vk::Image image) const override;
    };
}
