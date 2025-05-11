#pragma once

#include <torpedo/rendering/Camera.h>

#include <numbers>

namespace tpd {
    class PerspectiveCamera final : public Camera {
    public:
        PerspectiveCamera(uint32_t imageWidth, uint32_t imageHeight);

        void setVerticalFov(float degrees) noexcept;

        [[nodiscard]] const float* getProjectionData() const noexcept override;
        [[nodiscard]] uint32_t getProjectionByteSize() const noexcept override;

    private:
        void onImageSizeChange(uint32_t w, uint32_t h) noexcept override;
        void updateProjectionMatrix(float fy) noexcept;

        float _aspect;
        mat4 _projection;
    };
} // namespace tpd

inline tpd::PerspectiveCamera::PerspectiveCamera(const uint32_t imageWidth, const uint32_t imageHeight)
    : _aspect{ static_cast<float>(imageWidth) / static_cast<float>(imageHeight) }
{
    constexpr auto fy = std::numbers::sqrt3_v<float>; // equivalent to vertical fov of 60 degrees
    updateProjectionMatrix(fy);
}

inline const float* tpd::PerspectiveCamera::getProjectionData() const noexcept {
    return _projection.data_ptr();
}

inline uint32_t tpd::PerspectiveCamera::getProjectionByteSize() const noexcept {
    return sizeof(float) * 16;
}

inline void tpd::PerspectiveCamera::onImageSizeChange(const uint32_t w, const uint32_t h) noexcept {
    _aspect = static_cast<float>(w) / static_cast<float>(h);
    updateProjectionMatrix(_projection[1, 1]);
}
