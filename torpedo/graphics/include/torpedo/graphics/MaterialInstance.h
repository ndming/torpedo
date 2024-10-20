#pragma once

#include <torpedo/foundation/ShaderInstance.h>

namespace tpd {
    class Material;
    class Renderer;

    class MaterialInstance {
    public:
        MaterialInstance(std::unique_ptr<ShaderInstance> shaderInstance, const Material* material, uint32_t firstSet = 0) noexcept;

        MaterialInstance(const MaterialInstance& material) = delete;
        MaterialInstance& operator=(const MaterialInstance& material) = delete;

        virtual void bindDescriptorSets(vk::CommandBuffer buffer, uint32_t frameIndex) const;

        [[nodiscard]] const Material* getMaterial() const;

        static const std::unique_ptr<ShaderInstance>& getSharedShaderInstance();

        vk::CullModeFlagBits cullMode{ vk::CullModeFlagBits::eBack };
        vk::PolygonMode polygonMode{ vk::PolygonMode::eFill };
        float lineWidth{ 1.0f };

        void dispose(const Renderer& renderer) noexcept;

        virtual ~MaterialInstance() = default;

    private:
        std::unique_ptr<ShaderInstance> _shaderInstance;
        const Material* _material;
        const uint32_t _firstSet;

        // A ShaderInstance holding one shared descriptor set for each in-flight frames
        static std::unique_ptr<ShaderInstance> _sharedShaderInstance;
        friend class Renderer;
    };
}

inline std::unique_ptr<tpd::ShaderInstance> tpd::MaterialInstance::_sharedShaderInstance = {};

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::MaterialInstance::MaterialInstance(
    std::unique_ptr<ShaderInstance> shaderInstance,
    const Material* material,
    const uint32_t firstSet) noexcept
    : _shaderInstance{ std::move(shaderInstance) }
    , _material { material }
    , _firstSet{ firstSet } {
}

inline const tpd::Material* tpd::MaterialInstance::getMaterial() const {
    return _material;
}

inline const std::unique_ptr<tpd::ShaderInstance>& tpd::MaterialInstance::getSharedShaderInstance() {
    return _sharedShaderInstance;
}
