#pragma once

#include <vulkan/vulkan.hpp>

#include <filesystem>

namespace tpd {
    class ShaderModuleBuilder {
    public:
        ShaderModuleBuilder& spirvPath(const std::filesystem::path& path);

        ShaderModuleBuilder& slang(std::string_view assetsDir, const std::string& fileName);
        ShaderModuleBuilder& glsl (std::string_view assetsDir, const std::string& fileName);

        [[nodiscard]] vk::ShaderModule build(vk::Device device) const;

    private:
        std::vector<std::byte> _shaderCode{};
    };
}
