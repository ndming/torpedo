#pragma once

#include "torpedo/graphics/MaterialInstance.h"
#include "torpedo/graphics/Texture.h"

#include <torpedo/foundation/Buffer.h>

namespace tpd {
    class PhongMaterialInstance final : public MaterialInstance {
    public:
        PhongMaterialInstance(
            const Renderer& renderer,
            std::unique_ptr<ShaderInstance> shaderInstance,
            const Material* material);

        PhongMaterialInstance(const PhongMaterialInstance&) = delete;
        PhongMaterialInstance& operator=(const PhongMaterialInstance&) = delete;

        void activate(vk::CommandBuffer buffer, uint32_t frameIndex) const override;

        void setDiffuse(float r, float g, float b);
        void setDiffuse(const Texture& texture);

        void setSpecular(float r, float g, float b);
        void setSpecular(const Texture& texture);

        void setShininess(float s);

        void dispose() noexcept override;

        ~PhongMaterialInstance() override;

    private:
        struct MaterialObject {
            alignas(16) std::array<float, 3> diffuse{ 0.75164f, 0.60648f, 0.22648f };
            alignas(16) std::array<float, 3> specular{  0.628281f, 0.555802f, 0.366065f };
            alignas(4)  float shininess{ 51.2f };
            alignas(4)  uint32_t useDiffuseMap{ 0 };
            alignas(4)  uint32_t useSpecularMap{ 0 };
        };

        std::unique_ptr<Buffer> _materialObjectBuffer;
        MaterialObject _materialObject{};

        const ResourceAllocator* _allocator;
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline void tpd::PhongMaterialInstance::setDiffuse(const float r, const float g, const float b) {
    _materialObject.diffuse[0] = r;
    _materialObject.diffuse[1] = g;
    _materialObject.diffuse[2] = b;
    _materialObject.useDiffuseMap = 0;
}

inline void tpd::PhongMaterialInstance::setSpecular(const float r, const float g, const float b) {
    _materialObject.specular[0] = r;
    _materialObject.specular[1] = g;
    _materialObject.specular[2] = b;
    _materialObject.useSpecularMap = 0;
}

inline void tpd::PhongMaterialInstance::setShininess(const float s) {
    _materialObject.shininess = s;
}
