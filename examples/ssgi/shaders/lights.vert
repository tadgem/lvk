#version 450

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexUV;

layout(location = 0) out vec2 UV;

void main() {
    gl_Position = vec4(vertexPosition, 1.0);
    UV = vertexUV;
}