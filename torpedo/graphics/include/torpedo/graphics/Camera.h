#pragma once

#include <torpedo/foundation/Buffer.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <concepts>

namespace tpd {
    class Camera {
    public:
        Camera(uint32_t filmWidth, uint32_t filmHeight);

        Camera(const Camera&) = delete;
        Camera& operator=(const Camera&) = delete;

        void lookAt(const glm::vec3& eye, const glm::vec3& target, const glm::vec3& up);

        void setViewMatrix(const glm::mat4& viewMatrix);
        void setProjection(const glm::mat4& projection);

        [[nodiscard]] const glm::mat4& getViewMatrix() const;
        [[nodiscard]] const glm::mat4& getNormMatrix() const;
        [[nodiscard]] const glm::mat4& getProjection() const;

        struct CameraObject {
            alignas(16) glm::mat4 view;
            alignas(16) glm::mat4 norm;
            alignas(16) glm::mat4 proj;
        };
        static const std::unique_ptr<Buffer>& getCameraObjectBuffer();

        virtual ~Camera() = default;

    protected:
        glm::mat4 _view{ 1.0f };
        glm::mat4 _proj{ 1.0f };

    private:
        void updateNormalMatrix();
        glm::mat4 _norm{ 1.0f };

        static std::unique_ptr<Buffer> _cameraObjectBuffer;
        friend class Renderer;
    };

    template<typename T>
    concept Projectable = std::derived_from<T, Camera>;
}

inline std::unique_ptr<tpd::Buffer> tpd::Camera::_cameraObjectBuffer = {};

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::Camera::Camera([[maybe_unused]] const uint32_t filmWidth, [[maybe_unused]] const uint32_t filmHeight) {
    _view = glm::mat4(1.0f);
    _proj = glm::mat4(1.0f);
}

inline void tpd::Camera::lookAt(const glm::vec3& eye, const glm::vec3& target, const glm::vec3& up) {
    _view = glm::lookAt(eye, target, up);
    updateNormalMatrix();
}

inline void tpd::Camera::setViewMatrix(const glm::mat4& viewMatrix) {
    _view = viewMatrix;
    updateNormalMatrix();
}

inline void tpd::Camera::updateNormalMatrix() {
    _norm = transpose(inverse(_view));
}

inline void tpd::Camera::setProjection(const glm::mat4& projection) {
    _proj = projection;
}

inline const glm::mat4& tpd::Camera::getViewMatrix() const {
    return _view;
}

inline const glm::mat4& tpd::Camera::getNormMatrix() const {
    return _norm;
}

inline const glm::mat4& tpd::Camera::getProjection() const {
    return _proj;
}

inline const std::unique_ptr<tpd::Buffer>& tpd::Camera::getCameraObjectBuffer() {
    return _cameraObjectBuffer;
}
