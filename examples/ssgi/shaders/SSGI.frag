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

layout(binding = 0) uniform sampler2D lightPassColourEmissive;
layout(binding = 1) uniform sampler2D lightPassNormalDepth;

// this counts the number of 1s in the binary representation of val
int CountBits(int val)
{
    val = (val&0x55555555)+((val>>1)&0x55555555);
    val = (val&0x33333333)+((val>>2)&0x33333333);
    val = (val&0x0F0F0F0F)+((val>>4)&0x0F0F0F0F);
    val = (val&0x00FF00FF)+((val>>8)&0x00FF00FF);
    val = (val&0x0000FFFF)+((val>>16)&0x0000FFFF);
    return val;
}

void main() {
    vec4 Output = vec4(0.0);
    Output.w = 1.0;

    vec2 ASPECT = vec2(resolutionFov.x / resolutionFov.y, 1.0);
    float CFOV = tan(resolutionFov.z);
    
    float CurrentFrame = float(FrameIndex);
    vec2 SSOffset = vec2(0.);
    vec3 Tan; 
    vec3 Bit = TBN(eyeRotation.xyz,Tan);
    mat3 EyeMat = TBN(eyeRotation.xyz);
    vec3 Dir = normalize(vec3(((fragCoord+SSOffset)*IRES*2.-1.)*(ASPECT*CFOV),1.)*EyeMat);
    vec4 ColourAttr = texture(lightPassColourEmissive,UV);
    vec4 NormalDepthAttr = texture(lightPassNormalDepth,UV);

    vec3 PPos = eyePos.xyz+Dir*NormalDepthAttr.w;
    vec3 Normal = NormalDepthAttr.xyz;
    vec3 VNormal = vec3(dot(Normal,Tan),dot(Normal,Bit),dot(Normal,eyeRotation.xyz));
    vec3 VPPos = vec3(dot(PPos-eyePos.xyz,Tan),dot(PPos-eyePos.xyz,Bit),dot(PPos-eyePos.xyz,eyeRotation.xyz));
    vec2 ModFC = mod(fragCoord,4.);
    float RandPhiOffset = ARand21(vec2(1.234)+mod(CurrentFrame*3.26346,7.2634));
    float RandPhi = (mod(floor(ModFC.x)+floor(ModFC.y)*4.+CurrentFrame*5.,16.)+RandPhiOffset)*2.*PI*I64;

    for(int i = 0; i < 4; i++)
    {
        RandPhi += PI*0.5;
        vec2 SSDir = vec2(cos(RandPhi),sin(RandPhi));
        float StepDist = 1.;
        float StepCoeff = 0.15+0.15*ARand21(UV*(1.4+mod(float(FrameIndex)*3.26346,6.2634)));
        int BitMask = int(0);
        for(int i = 0; i < STEP_COUNT; i++)
        {
            vec2 SUV = UV+(SSDir*StepDist * IRES);
            float CurrentStep = max(1.,StepDist*StepCoeff);
            StepDist += CurrentStep;
            if(SUV.x > 1.0 || SUV.y > 1.0) break;
            vec4 SColourAttr = texture(lightPassColourEmissive,SUV);
            if(length(SColourAttr.yzw) <= LightCoeff) continue;
            vec4 SNormalDepthAttr = texture(lightPassNormalDepth,SUV);
            vec3 SVPPos = normalize(vec3((SUV*IRES*2.-1.)*(ASPECT*CFOV),1.))*SNormalDepthAttr.w;
            float NorDot = dot(VNormal,SVPPos-VPPos)-0.001;
            float TanDist = length(SVPPos-VPPos-NorDot*VNormal);
            float Angle1f = atan(NorDot,TanDist);
            float Angle2f = atan(NorDot-0.03*max(1.,StepDist*0.07),TanDist);
            float Angle1 = max(0.,ceil(Angle1f/(PI*0.5)*32.));
            float Angle2 = max(0.,floor(Angle2f/(PI*0.5)*32.));
            int SBitMask = (int(pow(2.,Angle1-Angle2))-1) << int(Angle2);
            vec3 SNormal = SNormalDepthAttr.xyz*2.-1.;
            SNormal = vec3(dot(SNormal,Tan),dot(SNormal,Bit),dot(SNormal,eyeRotation.xyz));
            Output.xyz += float(CountBits(SBitMask & (~BitMask)))/max(1.,Angle1-Angle2)*SColourAttr.yzw*LightCoeff
                                *(pow(cos(Angle2*I64*PI),2.)-pow(cos(Angle1*I64*PI),2.))
                                *sqrt(max(0.,dot(SNormal,-normalize(SVPPos-VPPos))));
            
            BitMask = BitMask | SBitMask;
        }
    }
    outColor = Output;
}