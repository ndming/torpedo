#pragma once

#include "torpedo/graphics/Camera.h"

namespace tpd {
    class PerspectiveCamera final : public Camera {
    public:
        PerspectiveCamera(uint32_t filmWidth, uint32_t filmHeight);

        void setProjection(float fov, float aspect, float near, float far);
    };
}
