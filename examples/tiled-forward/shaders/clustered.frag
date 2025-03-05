#version 450

#include "include/Defs.glsl"
#include "include/Lights.glsl"
#include "include/Helpers.glsl"

layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 UV;
layout(location = 3) in vec4 ClipPos;
layout(location = 4) in vec4 LastClipPos;

layout(location = 0) out vec4 outColor;
layout(location = 1) out uvec4 outPacking;

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

layout(set = 0, binding = 2) uniform LightsUBO{
    DirectionalLight    u_DirectionalLight;
    PointLight          u_PointLights[MAX_NUM_EACH_LIGHTS];
    SpotLight           u_SpotLights[MAX_NUM_EACH_LIGHTS];

    uint                u_DirLightActive;
    uint                u_PointLightsActive;
    uint                u_SpotLightsActive;
} lights_ubo;


layout(binding = 3) uniform sampler2D u_diffuse;
layout(binding = 4) uniform sampler2D u_normal_map;
layout(binding = 5) uniform sampler2D u_metallic;
layout(binding = 6) uniform sampler2D u_roughness;
layout(binding = 7) uniform sampler2D u_ao;
// depth to reconstruct last frame position
layout(binding = 8) uniform sampler2D u_previous_depth_map;

vec3 getNormalFromMap() {
    vec3 tangentNormal = texture(u_normal_map, UV).xyz * 2.0 - 1.0;

    if(abs(tangentNormal.z) < 0.0001) {
        tangentNormal = UnpackNormalMap(tangentNormal);
    }

    vec3 Q1 = dFdx(Position);
    vec3 Q2 = dFdy(Position);

    vec2 st1 = dFdx(UV);
    vec2 st2 = dFdy(UV);

    vec3 N = normalize(Normal);
    vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
    vec3 B = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}


void main() {
    vec4 inDiffuse = texture(u_diffuse, UV);
    if(inDiffuse.w < 0.25)
    {
        discard;
    }

    vec3 Diffuse = pow(inDiffuse.xyz, vec3(2.2));
    vec3 Normal = getNormalFromMap();

    float r = ((instance_ubo.u_entity_index & 0x000000FF) >>  0);
    float g = ((instance_ubo.u_entity_index & 0x0000FF00) >>  8);
    float b = ((instance_ubo.u_entity_index & 0x00FF0000) >> 16);

    vec3 EntityID = vec3(r,g,b);

    vec2 currentPosNDC = ClipPos.xy / ClipPos.z;
    vec2 previousPosNDC = LastClipPos.xy / LastClipPos.z;

    // velocity
    vec2 Velocity = currentPosNDC - previousPosNDC;
    float metallic = texture(u_metallic, UV).r;

    float roughness = 1.0;
    if(textureSize(u_roughness, 0).x > 0)
    {
        roughness = texture(u_roughness, UV).g;
    }

    float ao = texture(u_ao, UV).r;
    vec3 PBR = vec3(metallic, roughness, ao);
}