#pragma once

#include <torpedo/foundation/ResourceAllocator.h>
#include <torpedo/foundation/ShaderInstance.h>
#include <torpedo/foundation/Image.h>

namespace tpd {
    class Material;
    class Renderer;

    class MaterialInstance {
    public:
        MaterialInstance(std::unique_ptr<ShaderInstance> shaderInstance, const Material* material) noexcept;

        MaterialInstance(const MaterialInstance& material) = delete;
        MaterialInstance& operator=(const MaterialInstance& material) = delete;

        virtual void activate(vk::CommandBuffer buffer, uint32_t frameIndex) const;

        [[nodiscard]] const Material* getMaterial() const;

        static const std::unique_ptr<ShaderInstance>& getSharedShaderInstance();

        vk::CullModeFlagBits cullMode{ vk::CullModeFlagBits::eBack };
        vk::PolygonMode polygonMode{ vk::PolygonMode::eFill };
        float lineWidth{ 1.0f };

        virtual void dispose() noexcept;

        virtual ~MaterialInstance();

    protected:
        std::unique_ptr<ShaderInstance> _shaderInstance;
        const Material* _material;

        static const ResourceAllocator* _allocator;
        static std::unique_ptr<Image> _dummyImage;

    private:
        // A ShaderInstance holding one shared descriptor set for each in-flight frames
        static std::unique_ptr<ShaderInstance> _sharedShaderInstance;
        friend class Renderer;
    };
}

inline const tpd::ResourceAllocator* tpd::MaterialInstance::_allocator = nullptr;
inline std::unique_ptr<tpd::Image> tpd::MaterialInstance::_dummyImage = {};
inline std::unique_ptr<tpd::ShaderInstance> tpd::MaterialInstance::_sharedShaderInstance = {};

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::MaterialInstance::MaterialInstance(std::unique_ptr<ShaderInstance> shaderInstance, const Material* material) noexcept
    : _shaderInstance{ std::move(shaderInstance) }, _material { material } {
}

inline const tpd::Material* tpd::MaterialInstance::getMaterial() const {
    return _material;
}

inline const std::unique_ptr<tpd::ShaderInstance>& tpd::MaterialInstance::getSharedShaderInstance() {
    return _sharedShaderInstance;
}
