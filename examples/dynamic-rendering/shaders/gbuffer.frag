#version 450

layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 UV;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outPosition;
layout(location = 2) out vec4 outNormal;

layout(binding = 1) uniform sampler2D texSampler;

void main() {
    outColor = vec4(texture(texSampler, UV).rgb, 1.0);
    outPosition = vec4(Position, 1.0);
    outNormal = vec4(Normal, 1.0);
}