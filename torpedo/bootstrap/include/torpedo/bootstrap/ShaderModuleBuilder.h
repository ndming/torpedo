#pragma once

#include <vulkan/vulkan.hpp>

#include <filesystem>

namespace tpd {
    class ShaderModuleBuilder {
    public:
        ShaderModuleBuilder& spirvPath(const std::filesystem::path& path);

        [[nodiscard]] vk::ShaderModule build(vk::Device device) const;

    private:
        std::vector<std::byte> _shaderCode{};
    };
}
