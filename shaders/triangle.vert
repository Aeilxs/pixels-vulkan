#version 450

layout(location = 0) in vec2 position;
layout(location = 0) out vec3 fragmentColor;

layout(location = 1) in vec3 color;

layout(push_constant) uniform PushConstants {
    mat4 viewProjection;
}
pushConstants;

void main() {
    gl_Position = pushConstants.viewProjection * vec4(position, 0.0, 1.0);
    fragmentColor = color;
}
