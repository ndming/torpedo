#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragUV;

layout(set = 0, binding = 0) uniform DrawableObject {
    mat4 transform;
    mat4 normalMat;
} drawable;

layout(set = 0, binding = 1) uniform CameraObject {
    mat4 viewMatrix;
    mat4 viewNormal;
    mat4 projection;
} camera;

void main() {
    // Forward vertex attributes
    vec4 viewPos = camera.viewMatrix * drawable.transform * vec4(inPosition, 1.0);
    fragPosition = vec3(viewPos);
    fragNormal   = mat3(drawable.normalMat) * inNormal;
    fragUV       = inUV;

    // Compute vertex position
    gl_Position = camera.projection * viewPos;
}