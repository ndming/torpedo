#version 450

// Tell glslc that we want std430 layout for uniform buffer objects containing arrays
#extension GL_EXT_scalar_block_layout : enable

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform CameraObject {
    mat4 viewMatrix;
    mat4 viewNormal;
    mat4 projection;
} camera;

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

layout(set = 1, binding = 0) uniform MaterialObject {
    vec3 diffuse;
    vec3 specular;
    float shininess;
    uint useDiffuseMap;
    uint useSpecularMap;
} material;

layout(set = 1, binding = 1) uniform sampler2D diffuseMapSampler;
layout(set = 1, binding = 2) uniform sampler2D specularMapSampler;

vec3 calculateDiffuse(AmbientLight ambientLight);
vec3 calculateDiffuse(DistantLight distantLight, vec3 toLight);
vec3 calculateDiffuse(vec3 lightPos, vec3 lightColor, float attenuation);

vec3 calculateSpecular(AmbientLight ambientLight);
vec3 calculateSpecular(DistantLight distantLight, vec3 toLight);
vec3 calculateSpecular(vec3 lightPos, vec3 lightColor, float attenuation);

const float DIFFUSE_SCALE  = 0.5;
const float SPECULAR_SCALE = 1.0;

void main() {
    vec3 kD = vec3(0.0);
    vec3 kS = vec3(0.0);

    // Loop through ambient lights
    for (uint i = 0; i < light.ambientLightCount; i++) {
        kD += calculateDiffuse(light.ambientLights[i]);
        kS += calculateSpecular(light.ambientLights[i]);
    }

    // Loop through distant lights
    for (uint i = 0; i < light.distantLightCount; i++) {
        vec3 toLight = -normalize(mat3(camera.viewNormal) * light.distantLights[i].direction);
        kD += calculateDiffuse(light.distantLights[i], toLight);
        kS += calculateSpecular(light.distantLights[i], toLight);
    }
    // Loop through point lights
    for (uint i = 0; i < light.pointLightCount; i++) {
        vec3  lightPos = vec3(camera.viewMatrix * vec4(light.pointLights[i].position, 1.0));
        float distance = length(lightPos - fragPosition);
        float attenuation = light.pointLights[i].intensity / (1.0 + light.pointLights[i].decay * distance * distance);
        kD += calculateDiffuse(lightPos, light.pointLights[i].color, attenuation);
        kS += calculateSpecular(lightPos, light.pointLights[i].color, attenuation);
    }

    vec3 phongShading = kD + kS;
    outColor = vec4(phongShading, 1.0);
}

vec3 calculateDiffuse(AmbientLight ambientLight) {
    // Light diffuse component
    vec3 diffuse = ambientLight.intensity * ambientLight.color * DIFFUSE_SCALE;
    // Material diffuse component
    diffuse *= (material.useDiffuseMap != 0u) ? vec3(texture(diffuseMapSampler, fragUV)) : material.diffuse;
    return diffuse;
}

vec3 calculateDiffuse(DistantLight distantLight, vec3 toLight) {
    // Light diffuse component
    float cosine = max(dot(normalize(fragNormal), toLight), 0.0);
    vec3 diffuse = distantLight.intensity * distantLight.color * cosine * DIFFUSE_SCALE;
    // Material diffuse component
    diffuse *= (material.useDiffuseMap != 0u) ? vec3(texture(diffuseMapSampler, fragUV)) : material.diffuse;
    return diffuse;
}

vec3 calculateDiffuse(vec3 lightPos, vec3 lightColor, float attenuation) {
    // Light diffuse component
    vec3 toLight = normalize(lightPos - fragPosition);
    float cosine = max(dot(normalize(fragNormal), toLight), 0.0);
    vec3 diffuse = attenuation * lightColor * cosine * DIFFUSE_SCALE;
    // Material diffuse component
    diffuse *= (material.useDiffuseMap != 0u) ? vec3(texture(diffuseMapSampler, fragUV)) : material.diffuse;
    return diffuse;
}

vec3 calculateSpecular(AmbientLight ambientLight) {
    // Light specular component
    vec3 specular = ambientLight.intensity * ambientLight.color * SPECULAR_SCALE;
    // Material specular component
    specular *= (material.useSpecularMap != 0u) ? vec3(texture(specularMapSampler, fragUV)) : material.specular;
    return specular;
}

vec3 calculateSpecular(DistantLight distantLight, vec3 toLight) {
    // Light specular component
    vec3 toView = normalize(-fragPosition);
    vec3 toSpec = reflect(-toLight, normalize(fragNormal));
    float coefficient = pow(max(dot(toSpec, toView), 0.0f), material.shininess);
    vec3 specular = coefficient * distantLight.intensity * distantLight.color * SPECULAR_SCALE;
    // Material specular component
    specular *= (material.useSpecularMap != 0u) ? vec3(texture(specularMapSampler, fragUV)) : material.specular;
    return specular;
}

vec3 calculateSpecular(vec3 lightPos, vec3 lightColor, float attenuation) {
    // Light specular component
    vec3 toView = normalize(-fragPosition);
    vec3 toSpec = reflect(normalize(fragPosition - lightPos), normalize(fragNormal));
    float coefficient = pow(max(dot(toSpec, toView), 0.0f), material.shininess);
    vec3 specular = coefficient * attenuation * lightColor * SPECULAR_SCALE;
    // Material specular component
    specular *= (material.useSpecularMap != 0u) ? vec3(texture(specularMapSampler, fragUV)) : material.specular;
    return specular;
}
