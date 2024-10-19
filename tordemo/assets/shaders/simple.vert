#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in uint inInstance;

layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform CameraObject {
    mat4 viewMatrix;
    mat4 projection;
} camera;

void main() {
    vec3 pos = inPosition;
    pos.x += float(inInstance) * 0.25;
    gl_Position = camera.projection * camera.viewMatrix * vec4(pos, 1.0);
    fragColor = inColor;
}