#pragma once

#include <torpedo/foundation/ShaderInstance.h>

namespace tpd {
    class Material;

    class MaterialInstance {
    public:
        MaterialInstance(std::unique_ptr<ShaderInstance> shaderInstance, const Material* material) noexcept;

        MaterialInstance(const MaterialInstance& material) = delete;
        MaterialInstance& operator=(const MaterialInstance& material) = delete;

        virtual void activate(vk::CommandBuffer buffer, uint32_t frameIndex) const;

        [[nodiscard]] const Material* getMaterial() const;

        vk::CullModeFlagBits cullMode{ vk::CullModeFlagBits::eBack };
        vk::PolygonMode polygonMode{ vk::PolygonMode::eFill };
        float lineWidth{ 1.0f };

        virtual void dispose() noexcept;

        virtual ~MaterialInstance();

    protected:
        std::unique_ptr<ShaderInstance> _shaderInstance;
        const Material* _material;
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::MaterialInstance::MaterialInstance(std::unique_ptr<ShaderInstance> shaderInstance, const Material* material) noexcept
    : _shaderInstance{ std::move(shaderInstance) }, _material { material } {
}

inline const tpd::Material* tpd::MaterialInstance::getMaterial() const {
    return _material;
}
