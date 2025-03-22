#pragma once

#include <torpedo/material/PhongMaterial.h>

#include <assimp/scene.h>

class ModelLoader {
public:
    ModelLoader(const std::filesystem::path& file, const tpd::Renderer& renderer);

    std::unique_ptr<tpd::PhongMaterial> material{};
    std::shared_ptr<tpd::Drawable> drawable{};

private:
    std::shared_ptr<tpd::Drawable> processNode(const aiNode* node, const tpd::Renderer& renderer);
    tpd::Drawable::Mesh processMesh(const aiMesh* mesh, const tpd::Renderer& renderer);

    const aiScene* _aiScene;
    std::filesystem::path _directory;

    uint32_t loadTexture(const std::filesystem::path& path, const tpd::Renderer& renderer);
    std::vector<std::unique_ptr<tpd::Texture>> _textures{};
    std::unordered_map<std::string, uint32_t> _loadedTextures{};
};
