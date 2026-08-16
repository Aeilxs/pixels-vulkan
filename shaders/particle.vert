#version 450

layout(location = 0) in vec2 position;
layout(location = 1) in vec4 color;

layout(location = 0) out vec4 fragmentColor;

layout(push_constant) uniform PushConstants {
    mat4 viewProjection;
}
pushConstants;

void main() {
    gl_Position = pushConstants.viewProjection * vec4(position, 0.0, 1.0);

    gl_PointSize = 5.0;

    fragmentColor = color;
}