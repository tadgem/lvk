#version 450

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexVelocity;
layout(location = 2) in vec4 vertexColour;

layout(location = 0) out vec3 Position;
layout(location = 1) out vec3 Normal;
layout(location = 2) out vec4 Colour;


layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {

    Colour = vertexColour;

    mat4 normalMatrix = transpose(inverse(ubo.model));    
    Normal = normalize((normalMatrix * vec4(vertexVelocity, 0)).xyz);

    Position = (ubo.model * vec4(vertexPosition, 1.0)).xyz;
    gl_Position = ubo.proj * ubo.view * vec4(Position, 1.0);
}