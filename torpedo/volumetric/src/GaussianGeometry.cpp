#include "torpedo/volumetric/GaussianGeometry.h"

#include "miniply.h"

#include <random>
#include <ranges>
#include <fstream>

std::vector<tpd::GaussianPoint> tpd::GaussianPoint::random(
    const uint32_t count,
    const float radius,
    const vec3& center,
    const float minScale,
    const float maxScale,
    const float minOpacity,
    const float maxOpacity)
{
    std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution dist_pos(-1.0f, 1.0f);
    std::uniform_real_distribution dist_opacity(minOpacity, maxOpacity);
    std::uniform_real_distribution dist_scale(minScale, maxScale);
    std::uniform_real_distribution dist_sh(0.0f, 1.0f);

    auto points = std::vector<GaussianPoint>(count);
    for (auto& [position, opacity, quaternion, scale, sh] : points) {
        position = vec3{ dist_pos(rng), dist_pos(rng), dist_pos(rng) } * radius + center;
        opacity = dist_opacity(rng);
        quaternion = { 0.0f, 0.0f, 0.0f, 1.0f };
        scale = { dist_scale(rng), dist_scale(rng), dist_scale(rng), 1.0f };
        sh = utils::rgb2sh(dist_sh(rng), dist_sh(rng), dist_sh(rng));
    }
    return points;
}

std::vector<char> readBinaryFile(const std::filesystem::path& file) {
    // Reading from the end of the file as binary format to determine the file size
    auto bin = std::ifstream{ file, std::ios::ate | std::ios::binary };
    if (!bin.is_open()) {
        throw std::runtime_error("Failed to open file: " + file.string());
    }

    const auto fileSize = static_cast<size_t>(bin.tellg());
    auto buffer = std::vector<char>(fileSize);

    bin.seekg(0);
    bin.read(buffer.data(), static_cast<std::streamsize>(fileSize));
    bin.close();

    return buffer;
}

uint32_t countFeatures(const miniply::PLYElement* info) noexcept {
    uint32_t count = 0;
    for (const auto& property : info->properties) {
        if (property.name.starts_with("f_rest_")) ++count;
    }
    return count + 3; // 3 DC components
}

std::vector<tpd::GaussianPoint> tpd::GaussianPoint::fromModel(const std::filesystem::path& plyFile) {
    using namespace miniply;
    auto file = PLYReader{ plyFile.string().c_str() };
    if (!file.valid()) {
        throw std::runtime_error("Failed to open file: " + plyFile.string());
    }

    const PLYElement* info = file.get_element(file.find_element(kPLYVertexElement));
    if (!info) throw std::runtime_error("Could NOT find vertices: " + plyFile.string());

    const auto pointCount = info->count;

    const auto positionData = new float[pointCount * 3];
    const auto rotationData = new float[pointCount * 4];
    const auto scaleData    = new float[pointCount * 3];
    const auto opacityData  = new float[pointCount];

    const auto featureCount = countFeatures(info);
    const auto featureData  = new float[pointCount * featureCount];

    uint32_t positionIndices[3];
    uint32_t rotationIndices[4];
    uint32_t scaleIndices[3];
    uint32_t opacityIndices;
    uint32_t featureIndices[MAX_SH_FLOATS];
    info->find_properties(positionIndices, 3, "x", "y", "z");
    info->find_properties(rotationIndices, 4, "rot_0", "rot_1", "rot_2", "rot_3");
    info->find_properties(scaleIndices, 3, "scale_0", "scale_1", "scale_2");
    info->find_properties(&opacityIndices, 1, "opacity");
    info->find_properties(featureIndices, 3, "f_dc_0", "f_dc_1", "f_dc_2");

    // Assuming extra features are laid out contiguously from DC
    for (auto i = 3; i < featureCount; ++i) {
        featureIndices[i] = featureIndices[i - 1] + 1;
    }

    while (file.has_element()) {
        if (file.element_is(kPLYVertexElement) && file.load_element()) [[likely]] {
            file.extract_properties(positionIndices, 3, PLYPropertyType::Float, positionData);
            file.extract_properties(rotationIndices, 4, PLYPropertyType::Float, rotationData);
            file.extract_properties(scaleIndices, 3, PLYPropertyType::Float, scaleData);
            file.extract_properties(&opacityIndices, 1, PLYPropertyType::Float, opacityData);
            file.extract_properties(featureIndices, featureCount, PLYPropertyType::Float, featureData);
            break;
        }
    }

    auto points = std::vector<GaussianPoint>{};
    points.resize(pointCount);

    auto idx = 0;
    for (auto& [position, opacity, quaternion, scale, sh] : points) {
        position = { positionData[idx * 3 + 0], positionData[idx * 3 + 1], positionData[idx * 3 + 2] };
        opacity = opacityData[idx];
        quaternion = { rotationData[idx * 4 + 1], rotationData[idx * 4 + 2], rotationData[idx * 4 + 3], rotationData[idx * 4 + 0] };
        scale = { scaleData[idx * 3 + 0], scaleData[idx * 3 + 1], scaleData[idx * 3 + 2], 1.0f };
        memcpy(sh.data(), featureData + idx * featureCount, sizeof(float) * featureCount);
        idx++;
    }

    delete[] positionData;
    delete[] rotationData;
    delete[] scaleData;
    delete[] opacityData;
    delete[] featureData;

    return points;
}
