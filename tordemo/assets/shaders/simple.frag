#version 450

#extension GL_EXT_scalar_block_layout : enable

layout(location = 0) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

struct AmbientLight {
    vec3 color;
    float intensity;
};

struct DistantLight {
    vec3 direction;
    vec3 color;
    float intensity;
};

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float decay;
};

layout(std430, set = 0, binding = 2) uniform LightObject {
    uint ambientLightCount;
    uint distantLightCount;
    uint pointLightCount;
    AmbientLight ambientLights[1];
    DistantLight distantLights[8];
    PointLight pointLights[64];
} light;

void main() {
    outColor = vec4(light.ambientLights[0].color, 1.0);
}