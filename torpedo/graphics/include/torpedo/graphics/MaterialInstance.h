#pragma once

#include <torpedo/foundation/ShaderInstance.h>

namespace tpd {
    class MaterialInstance {
    public:
        vk::CullModeFlagBits cullMode{ vk::CullModeFlagBits::eBack };
        vk::PolygonMode polygonMode{ vk::PolygonMode::eFill };

    private:
    };
}
