#version 450

// Screen Space Horizon GI
// Adapted from https://www.shadertoy.com/view/dsGBzW
// which is an implementation of this paper:  https://arxiv.org/pdf/2301.11376.pdf

#include "SSGI.utils.glsl"
layout(location = 0) in vec2 UV;

layout(location = 0) out vec4 outColor;

layout(push_constant, std430) uniform pc {
    mat4 view;
    mat4 proj;
    vec4 resolutionFov;
    vec4 eyePos;
    vec4 eyeRotation;
    int FrameIndex;
};

#define fragCoord gl_FragCoord.xy
#define RES resolutionFov.xy
#define IRES 1.0 / resolutionFov.xy
#define STEP_COUNT 32

vec4 SampleMB(vec2 uv, vec3 PPos, vec3 Normal, vec3 LPos, mat3 LEyeMat, out float Weight) {
    //A weighted sample for a bilinear mix
    vec2 ClampedUV = clamp(uv, vec2(1.5), RES-0.5);
    vec4 C = texture(iChannel3, ClampedUV*IRES);
    vec3 SDir = normalize(vec3((uv*IRES*2.-1.)*(ASPECT*CFOV), 1.)*LEyeMat);
    Weight = max(0.0001, float(abs(dot(Normal, LPos+SDir*C.w-PPos))<0.01))*
    max(0.001, float(dot(Normal, normalize(FloatToVec3(C.z)*2.-1.))>0.9));
    return texture(iChannel2, ClampedUV*IRES)*Weight;
}

vec4 ManualBilinear(vec2 uv, vec3 PPos, vec3 Normal, vec3 LPos, mat3 LEyeMat, out float Weight) {
    //Returns weighted RGB sum + weight sum
    vec2 Fuv = floor(uv-0.4999)+0.5;
    float W0, W1, W2, W3;
    vec4 S0 = SampleMB(Fuv, PPos, Normal, LPos, LEyeMat, W0);
    vec4 S1 = SampleMB(Fuv+vec2(1., 0.), PPos, Normal, LPos, LEyeMat, W1);
    vec4 S2 = SampleMB(Fuv+vec2(0., 1.), PPos, Normal, LPos, LEyeMat, W2);
    vec4 S3 = SampleMB(Fuv+vec2(1.), PPos, Normal, LPos, LEyeMat, W3);
    vec2 fuv = fract(uv-0.4999);
    Weight = mix(mix(W0, W1, fuv.x), mix(W2, W3, fuv.x), fuv.y);
    return mix(mix(S0, S1, fuv.x), mix(S2, S3, fuv.x), fuv.y)/Weight;
}

void main() {
    vec4 Output = vec4(0.0, 0.0, 0.0, 1.0);

}