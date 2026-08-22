#version 450

layout(set = 0, binding = 0) uniform sampler2D fontAtlas;

layout(location = 0) in vec2 fragmentUv;
layout(location = 0) out vec4 outputColor;

void main() {
    float coverage = texture(fontAtlas, fragmentUv).r;
    outputColor = vec4(vec3(0.95), coverage);
}
