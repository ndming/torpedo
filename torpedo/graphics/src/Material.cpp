#include "torpedo/graphics/Material.h"
#include "torpedo/graphics/Renderer.h"

#include <fstream>
#include <ranges>

std::filesystem::path tpd::Material::getShaderFilePath(const std::string_view file) {
    return std::filesystem::path(TORPEDO_ASSETS_DIR) / "shaders" / file;
}

std::vector<std::byte> tpd::Material::Builder::readShaderFile(const std::filesystem::path& path) {
    // Reading from the end of the file as binary format
    auto file = std::ifstream{ path, std::ios::ate | std::ios::binary };
    if (!file.is_open()) {
        throw std::runtime_error("Material::Builder - Failed to open file: " + path.string());
    }

    // The advantage of starting to read at the end of the file is that we can use the read position to
    // determine the size of the file and allocate a buffer
    const auto fileSize = static_cast<size_t>(file.tellg());
    auto buffer = std::vector<char>(fileSize);

    // Seek back to the beginning of the file and read all the bytes at once
    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(fileSize));

    file.close();
    return buffer
        | std::views::transform([](const auto it) { return static_cast<std::byte>(it); })
        | std::ranges::to<std::vector>();
}

vk::ShaderModule tpd::Material::Builder::createShaderModule(const vk::Device device, const std::vector<std::byte>& code) {
    auto shaderModuleCreateInfo = vk::ShaderModuleCreateInfo{};
    shaderModuleCreateInfo.codeSize = static_cast<uint32_t>(code.size());
    shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    return device.createShaderModule(shaderModuleCreateInfo);
}

std::shared_ptr<tpd::MaterialInstance> tpd::Material::createInstance() const {
    // If this Material was not created with user-declared ShaderLayout, the first descriptor set (shared set)
    // has already been created and managed by the Renderer
    const auto firstSet = static_cast<uint32_t>(_withSharedLayout);
    auto shaderInstance = _shaderLayout->createInstance(_device, Renderer::MAX_FRAMES_IN_FLIGHT, firstSet);
    return std::make_shared<MaterialInstance>(std::move(shaderInstance), this);
}

void tpd::Material::dispose() noexcept {
    if (_shaderLayout) {
        _device.destroyPipeline(_pipeline);
        _shaderLayout->dispose(_device);
        _shaderLayout.reset();
    }
}

tpd::Material::~Material() {
    dispose();
}
