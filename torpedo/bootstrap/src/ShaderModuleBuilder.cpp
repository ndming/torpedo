#include "torpedo/bootstrap/ShaderModuleBuilder.h"

#include <fstream>
#include <ranges>

tpd::ShaderModuleBuilder& tpd::ShaderModuleBuilder::spirvPath(const std::filesystem::path& path) {
    // Reading from the end of the file as binary format to determine the file size
    auto file = std::ifstream{ path, std::ios::ate | std::ios::binary };
    if (!file.is_open()) {
        throw std::runtime_error("ShaderModuleBuilder - Failed to open file: " + path.string());
    }

    const auto fileSize = static_cast<size_t>(file.tellg());
    auto buffer = std::vector<char>(fileSize);

    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
    file.close();

    _shaderCode = buffer
        | std::views::transform([](const auto it) { return static_cast<std::byte>(it); })
        | std::ranges::to<std::vector>();

    return *this;
}

tpd::ShaderModuleBuilder& tpd::ShaderModuleBuilder::shader(const std::string_view assetsDir, const std::string& shaderFileName) {
    const auto path = std::filesystem::path(assetsDir) / "shaders" / (shaderFileName + ".spv");
    return spirvPath(path);
}

vk::ShaderModule tpd::ShaderModuleBuilder::build(const vk::Device device) const {
    if (_shaderCode.empty()) {
        throw std::runtime_error("ShaderModuleBuilder - Shader code is empty: did you forget to call ShaderModuleBuilder::spirvPath()?");
    }
    auto shaderModuleCreateInfo = vk::ShaderModuleCreateInfo{};
    shaderModuleCreateInfo.codeSize = static_cast<uint32_t>(_shaderCode.size());
    shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(_shaderCode.data());
    return device.createShaderModule(shaderModuleCreateInfo);
}
