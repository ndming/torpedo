#pragma once

#include <torpedo/foundation/Buffer.h>

namespace tpd {
    class Light {
    public:
        template <typename T, typename L>
        class Builder {
        public:
            T& color(float r, float g, float b);
            T& intensity(float intensity);

            virtual std::shared_ptr<L> build() const = 0;
            virtual ~Builder() = default;

        protected:
            float _r{ 1.0f };
            float _g{ 1.0f };
            float _b{ 1.0f };
            float _intensity{ 1.0f };
        };

        Light(std::array<float, 3> color, float intensity);

        void setColor(float r, float g, float b);

        std::array<float, 3> color;
        float intensity;

        static constexpr uint32_t MAX_AMBIENT_LIGHTS = 1;
        static constexpr uint32_t MAX_DISTANT_LIGHTS = 8;
        static constexpr uint32_t MAX_POINT_LIGHTS   = 64;

        struct AmbientLight {
            alignas(16) std::array<float, 3> color;
            alignas(4)  float intensity;
        };

        struct DistantLight {
            alignas(16) std::array<float, 3> direction;
            alignas(16) std::array<float, 3> color;
            alignas(4)  float intensity;
        };

        struct PointLight {
            alignas(16) std::array<float, 3> position;
            alignas(16) std::array<float, 3> color;
            alignas(4)  float intensity;
            alignas(4)  float decay;
        };

        struct LightObject {
            alignas(4)  uint32_t ambientLightCount;
            alignas(4)  uint32_t distantLightCount;
            alignas(4)  uint32_t pointLightCount;
            alignas(16) std::array<AmbientLight, MAX_AMBIENT_LIGHTS> ambientLights;
            alignas(16) std::array<DistantLight, MAX_DISTANT_LIGHTS> distantLights;
            alignas(16) std::array<PointLight, MAX_POINT_LIGHTS> pointLights;
        };

        static const std::unique_ptr<Buffer>& getLightObjectBuffer();

    private:
        static std::unique_ptr<Buffer> _lightObjectBuffer;
        friend class Renderer;
    };
}

inline std::unique_ptr<tpd::Buffer> tpd::Light::_lightObjectBuffer = {};

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

template<typename T, typename L>
T& tpd::Light::Builder<T, L>::color(const float r, const float g, const float b) {
    _r = r;
    _g = g;
    _b = b;
    return *static_cast<T*>(this);
}

template<typename T, typename L>
T& tpd::Light::Builder<T, L>::intensity(const float intensity) {
    _intensity = intensity;
    return *static_cast<T*>(this);
}

inline tpd::Light::Light(const std::array<float, 3> color, const float intensity) : color{ color }, intensity{ intensity } {
}

inline void tpd::Light::setColor(const float r, const float g, const float b) {
    color[0] = r;
    color[1] = g;
    color[2] = b;
}

inline const std::unique_ptr<tpd::Buffer>& tpd::Light::getLightObjectBuffer() {
    return _lightObjectBuffer;
}
