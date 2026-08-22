#version 450

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;

layout(location = 0) out vec2 fragmentUv;

layout(push_constant) uniform PushConstants {
    vec2 framebufferSize;
}
pushConstants;

void main() {
    vec2 normalized = position / pushConstants.framebufferSize;
    vec2 clipPosition = vec2(normalized.x * 2.0 - 1.0, normalized.y * 2.0 - 1.0);

    gl_Position = vec4(clipPosition, 0.0, 1.0);
    fragmentUv = uv;
}
