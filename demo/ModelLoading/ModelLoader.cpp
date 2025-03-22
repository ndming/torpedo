#include "ModelLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "stb.h"

ModelLoader::ModelLoader(const std::filesystem::path& file, const tpd::Renderer& renderer) {
    auto importer = Assimp::Importer{};

    // OBJ format assumes a coordinate system where a vertical coordinate of 0 means the bottom of the image,
    // however we’ll be using these images in a top to bottom orientation where 0 means the top of the image
    _aiScene = importer.ReadFile(file.string(), aiProcess_Triangulate);

    if(!_aiScene || _aiScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !_aiScene->mRootNode) {
        throw std::runtime_error("Assimp Error - " + std::string(importer.GetErrorString()));
    }
    _directory = file.parent_path();

    material = tpd::PhongMaterial::Builder().build(renderer);
    drawable = processNode(_aiScene->mRootNode, renderer);
}

std::shared_ptr<tpd::Drawable> ModelLoader::processNode(const aiNode* node, const tpd::Renderer& renderer) {
    auto drawableBuilder = tpd::Drawable::Builder().meshCount(node->mNumMeshes);
    for (uint32_t i = 0; i < node->mNumMeshes; ++i) {
        const auto mesh = _aiScene->mMeshes[node->mMeshes[i]];
        drawableBuilder.mesh(i, processMesh(mesh, renderer));
    }
    auto drawable = drawableBuilder.build();
    for (uint32_t i = 0; i < node->mNumChildren; ++i) {
        drawable->attach(processNode(node->mChildren[i], renderer));
    }
    return drawable;
}

tpd::Drawable::Mesh ModelLoader::processMesh(const aiMesh* mesh, const tpd::Renderer& renderer) {
    const auto geometry = tpd::Geometry::Builder()
        .vertexCount(mesh->mNumVertices)
        .indexCount(mesh->mNumFaces * 3)
        .build(renderer);

    auto positions = std::vector<float>(mesh->mNumVertices * 3);
    auto normals   = std::vector<float>(mesh->mNumVertices * 3);
    auto uvs       = std::vector<float>(mesh->mNumVertices * 2);

    for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
        positions[i * 3 + 0] = mesh->mVertices[i].x;
        positions[i * 3 + 1] = mesh->mVertices[i].y;
        positions[i * 3 + 2] = mesh->mVertices[i].z;

        normals[i * 3 + 0] = mesh->mNormals[i].x;
        normals[i * 3 + 1] = mesh->mNormals[i].y;
        normals[i * 3 + 2] = mesh->mNormals[i].z;

        uvs[i * 2 + 0] = mesh->mTextureCoords[0] ? mesh->mTextureCoords[0][i].x : 0.0f;
        uvs[i * 2 + 1] = mesh->mTextureCoords[0] ? mesh->mTextureCoords[0][i].y : 0.0f;
    }

    geometry->setVertexData("position", positions.data(), renderer);
    geometry->setVertexData("normal",   normals.data(),   renderer);
    geometry->setVertexData("uv",       uvs.data(),       renderer);

    auto indices = std::vector<uint32_t>(mesh->mNumFaces * 3);
    for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
        const auto face = mesh->mFaces[i];
        for (uint32_t j = 0; j < face.mNumIndices; j++) {
            indices[i * 3 + j] = face.mIndices[j];
        }
    }
    geometry->setIndexData(indices.data(), renderer);

    const auto mat = _aiScene->mMaterials[mesh->mMaterialIndex];
    const auto materialInstance = material->createInstance(renderer);

    if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
        aiString str;
        mat->GetTexture(aiTextureType_DIFFUSE, 0, &str);
        const auto texturePath  = _directory / str.C_Str();
        const auto textureIndex = loadTexture(texturePath, renderer);
        materialInstance->setDiffuse(*_textures[textureIndex]);
    }
    if (mat->GetTextureCount(aiTextureType_SPECULAR) > 0) {
        aiString str;
        mat->GetTexture(aiTextureType_SPECULAR, 0, &str);
        const auto texturePath  = _directory / str.C_Str();
        const auto textureIndex = loadTexture(texturePath, renderer);
        materialInstance->setSpecular(*_textures[textureIndex]);
    }

    return  tpd::Drawable::Mesh{ geometry, materialInstance, 1 };
}

uint32_t ModelLoader::loadTexture(const std::filesystem::path& path, const tpd::Renderer& renderer) {
    const auto texturePath = path.string();
    if (_loadedTextures.contains(texturePath)) {
        return _loadedTextures[texturePath];
    }

    int texWidth, texHeight, texChannels;
    const auto pixels = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    auto texture = tpd::Texture::Builder()
        .size(texWidth, texHeight)
        .build(renderer);
    texture->setImageData(pixels, texWidth, texHeight, renderer);

    stbi_image_free(pixels);

    _loadedTextures[texturePath] = static_cast<uint32_t>(_textures.size());
    _textures.push_back(std::move(texture));

    return _loadedTextures[texturePath];
}
