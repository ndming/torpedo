#pragma once

#include "torpedo/graphics/Material.h"
#include "torpedo/material/PhongMaterialInstance.h"

namespace tpd {
    class PhongMaterial final : public Material {
    public:
        class Builder {
        public:
            Builder& minSampleShading(float rate);

            [[nodiscard]] std::unique_ptr<PhongMaterial> build(const Renderer& renderer) const;

        private:
            float _minSampleShading{ 0.0f };
        };

        PhongMaterial(vk::Device device, vk::Pipeline pipeline, std::unique_ptr<ShaderLayout> shaderLayout, bool withSharedLayout = true) noexcept;

        PhongMaterial(const PhongMaterial&) = delete;
        PhongMaterial& operator=(const PhongMaterial&) = delete;

        [[nodiscard]] std::shared_ptr<PhongMaterialInstance> createInstance(const Renderer& renderer) const;
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::PhongMaterial::PhongMaterial(
    const vk::Device device,
    const vk::Pipeline pipeline,
    std::unique_ptr<ShaderLayout> shaderLayout,
    const bool withSharedLayout) noexcept
    : Material{ device, pipeline, std::move(shaderLayout), withSharedLayout } {
}
