#pragma once

#include <vulkan/vulkan.hpp>

#include <filesystem>

namespace tpd {
    class ShaderModuleBuilder {
    public:
        ShaderModuleBuilder& spirvPath(const std::filesystem::path& path);
        ShaderModuleBuilder& shader(std::string_view assetsDir, const std::string& shaderFileName);

        [[nodiscard]] vk::ShaderModule build(vk::Device device) const;

    private:
        std::vector<std::byte> _shaderCode{};
    };
}
