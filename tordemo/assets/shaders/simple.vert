#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in uint inInstance;

layout(location = 0) out vec4 fragColor;

void main() {
    vec3 pos = inPosition;
    pos.x += float(inInstance) * 0.25;
    gl_Position = vec4(pos, 1.0);
    fragColor = inColor;
}