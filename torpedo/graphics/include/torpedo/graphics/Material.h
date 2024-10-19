#pragma once

#include "torpedo/graphics/MaterialInstance.h"

#include <torpedo/foundation/ShaderLayout.h>

#include <filesystem>

namespace tpd {
    class Material {
    public:
        class Builder {
        public:
            Builder& vertShader(const std::filesystem::path& path, std::string entryPoint = "main");
            Builder& fragShader(const std::filesystem::path& path, std::string entryPoint = "main");

            [[nodiscard]] std::unique_ptr<Material> build(const Renderer& renderer, std::unique_ptr<ShaderLayout> layout = {}) const;

        private:
            static std::vector<std::byte> readShaderFile(const std::filesystem::path& path);
            static vk::ShaderModule createShaderModule(vk::Device device, const std::vector<std::byte>& code);

            std::vector<std::byte> _vertShaderCode{};
            std::vector<std::byte> _fragShaderCode{};

            std::string _vertShaderEntryPoint{};
            std::string _fragShaderEntryPoint{};
        };

        Material(vk::Pipeline pipeline, std::unique_ptr<ShaderLayout> shaderLayout, bool userDeclaredLayout = false) noexcept;

        Material(const Material&) = delete;
        Material& operator=(const Material&) = delete;

        [[nodiscard]] std::shared_ptr<MaterialInstance> createInstance(const Renderer& renderer) const;

        [[nodiscard]] vk::Pipeline getVulkanPipeline() const;
        [[nodiscard]] vk::PipelineLayout getVulkanPipelineLayout() const;

        void dispose(const Renderer& renderer) noexcept;

        virtual ~Material() = default;

    private:
        vk::Pipeline _pipeline;
        std::unique_ptr<ShaderLayout> _shaderLayout;
        bool _userDeclaredLayout;
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::Material::Builder& tpd::Material::Builder::vertShader(const std::filesystem::path& path, std::string entryPoint) {
    _vertShaderCode = readShaderFile(path);
    _vertShaderEntryPoint = std::move(entryPoint);
    return *this;
}

inline tpd::Material::Builder& tpd::Material::Builder::fragShader(const std::filesystem::path& path, std::string entryPoint) {
    _fragShaderCode = readShaderFile(path);
    _fragShaderEntryPoint = std::move(entryPoint);
    return *this;
}

inline tpd::Material::Material(const vk::Pipeline pipeline, std::unique_ptr<ShaderLayout> shaderLayout, const bool userDeclaredLayout) noexcept
: _pipeline{ pipeline }, _shaderLayout{ std::move(shaderLayout) }, _userDeclaredLayout { userDeclaredLayout } {
}

inline vk::Pipeline tpd::Material::getVulkanPipeline() const {
    return _pipeline;
}

inline vk::PipelineLayout tpd::Material::getVulkanPipelineLayout() const {
    return _shaderLayout->getPipelineLayout();
}
