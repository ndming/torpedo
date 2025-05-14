#pragma once

#include <torpedo/math/mat4.h>

namespace tpd {
    class Camera;

    template<typename T>
    concept CameraImpl = std::is_base_of_v<Camera, T> && std::is_final_v<T> && std::constructible_from<T, uint32_t, uint32_t>;

    class Camera {
    public:
        explicit Camera(const mat4& w2c = mat4{ 1.0f }) noexcept : _view{ w2c } {}
        Camera(const mat3& R, const vec3& t) noexcept : _view{ R, t } {}

        void lookAt(const vec3& eye, const vec3& center, const vec3& up) noexcept;
        void lookAt(const mat3& R, const vec3& t) noexcept;

        void setNear(float near) noexcept;
        void setFar(float far) noexcept;

        [[nodiscard]] const mat4& getViewMatrix() const& noexcept;
        [[nodiscard]] mat4 getViewMatrix() const&& noexcept;
        [[nodiscard]] const float* getViewMatrixData() const noexcept;

        [[nodiscard]] virtual const float* getProjectionData() const noexcept = 0;
        [[nodiscard]] virtual uint32_t getProjectionByteSize() const noexcept = 0;

        virtual void onImageSizeChange(uint32_t w, uint32_t h) noexcept {}
        virtual ~Camera() = default;

    protected:
        float _near{ 0.01f };
        float _far{ 100.0f };

    private:
        mat4 _view;
    };
} // namespace tpd

inline void tpd::Camera::setNear(const float near) noexcept {
    _near = near;
}

inline void tpd::Camera::setFar(const float far) noexcept {
    _far = far;
}

inline const tpd::mat4& tpd::Camera::getViewMatrix() const& noexcept {
    return _view;
}

inline tpd::mat4 tpd::Camera::getViewMatrix() const&& noexcept {
    return _view;
}

inline const float* tpd::Camera::getViewMatrixData() const noexcept {
    return _view.data_ptr();
}