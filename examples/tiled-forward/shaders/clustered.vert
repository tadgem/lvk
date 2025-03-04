#version 450

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexUV;

layout(location = 0) out vec3 Position;
layout(location = 1) out vec3 Normal;
layout(location = 2) out vec2 UV;
layout(location = 3) out vec4 ClipPos;
layout(location = 4) out vec4 LastClipPos;

layout(set = 0, binding = 0) uniform InstanceUBO {
    mat4 u_model;
    mat4 u_last_model;
    mat4 u_normal;
    uint u_entity_index;
} instance_ubo;

layout(set = 0, binding = 1) uniform SceneViewUBO {
    mat4    u_vp;
    mat4    u_last_vp;
    int     u_frame_index;
    vec2    u_resolution;
} scene_view_ubo;

const vec2 halton_seq[16] = vec2[16]
(
    vec2(0.500000, 0.333333),
    vec2(0.250000, 0.666667),
    vec2(0.750000, 0.111111),
    vec2(0.125000, 0.444444),
    vec2(0.625000, 0.777778),
    vec2(0.375000, 0.222222),
    vec2(0.875000, 0.555556),
    vec2(0.062500, 0.888889),
    vec2(0.562500, 0.037037),
    vec2(0.312500, 0.370370),
    vec2(0.812500, 0.703704),
    vec2(0.187500, 0.148148),
    vec2(0.687500, 0.481481),
    vec2(0.437500, 0.814815),
    vec2(0.937500, 0.259259),
    vec2(0.031250, 0.592593)
);

void main() {
    UV = vertexUV;
    Normal = (vec4(vertexNormal, 1.0) * instance_ubo.u_normal).xyz;
    Position = (instance_ubo.u_model * vec4(vertexPosition , 1.0)).xyz;
    vec4 pos =  scene_view_ubo.u_vp * instance_ubo.u_model * vec4(vertexPosition, 1.0);
    ClipPos = pos;

    LastClipPos = scene_view_ubo.u_last_vp *
        instance_ubo.u_last_model * vec4(vertexPosition, 1.0);

    int jitter_index = scene_view_ubo.u_frame_index % 16;
    vec2 offset = halton_seq[jitter_index];
    offset.x = ((offset.x-0.5) / scene_view_ubo.u_resolution.x) * 4.0;
    offset.y = ((offset.y-0.5) / scene_view_ubo.u_resolution.y ) * 4.0;

    pos += vec4(offset * pos.z, 0.0, 0.0);
    gl_Position = pos;
}