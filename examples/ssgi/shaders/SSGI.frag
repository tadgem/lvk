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
    int FrameIndex;
};

layout(binding = 0) uniform sampler2D positionBufferSampler;
layout(binding = 1) uniform sampler2D normalBufferSampler;
layout(binding = 2) uniform sampler2D lightPassImageSampler;
layout(binding = 3) uniform sampler2D depthImageSampler;

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
    vec4 result = vec4(0.0);
    result.w = 1.0;
    
    float CurrentFrame = FrameIndex;
    vec2 SSOffset = vec2(0.);
    vec2 fragCoord = gl_FragCoord.xy;
    vec2 IRES = 1.0 / resolutionFov.xy;
    float fov = resolutionFov[2];
    float CFOV = tan(fov);
    vec2 ASPECT = vec2(resolutionFov.x/resolutionFov.y,1.0);
    
    
    // sample gbuffer
    vec3 colour = (texture(lightPassImageSampler, UV) * proj * view).xyz;
    // vec3 position = texture(positionBufferSampler, UV).xyz;
    vec3 position = (texture(positionBufferSampler, UV) * proj * view).xyz;
    // vec3 normal = texture(normalBufferSampler, UV).xyz;
    vec3 normal = (texture(normalBufferSampler, UV) * proj * view).xyz;
    vec3 Tan = vec3(0.0);
    vec3 Bit = TBN(eyePos.xyz, Tan);
    mat3 EyeMat = TBN(eyePos.xyz);

    if(length(colour) > -0.5)
    {
        // Horizons (AKA wtf is happening)
        // vec3 VNormal = vec3(dot(normal, Tan), dot(normal, Bit), dot(normal, eyePos.xyz));
        vec3 VNormal = normal;
        // vec3 VPPos = vec3(dot(position, Tan), dot(position, Bit), dot(position, eyePos.xyz));
        vec3 VPPos = position;
        // vec2 ModFC = mod(fragCoord, 4.0);
    	vec2 ModFC = fragCoord;
        float RandPhiOffset = ARand21(vec2(1.234)+mod(CurrentFrame*3.26346,7.2634));
        float RandPhiMultiplier = 2 * PI * I64;
        float RandPhi = (mod(floor(ModFC.x)+floor(ModFC.y)*4.0+CurrentFrame*5.0, 16.0) + RandPhiOffset) * RandPhiMultiplier;
        
        for(int i = 0; i < 4; i++)
        {
            RandPhi += PI * 0.5;
            vec2 SSDir = vec2(cos(RandPhi), sin(RandPhi));
            float StepDist = 1.0;
            float StepCoeff = 0.15+0.15*ARand21(fragCoord*IRES*(1.4+mod(CurrentFrame *3.26346,6.2634)));
            int BitMask = 0;

            for (int s= 1; s < 32; s++) {
                vec2 SUV = fragCoord+SSDir*StepDist;
                float CurrentStep = max(1.,StepDist*StepCoeff);
                StepDist += CurrentStep;
                if (DFBox(SUV-1.,resolutionFov.xy-1.)>0.) break;
                vec2 SAttrUV = SUV * IRES;
                vec3 SAttrY = texture(lightPassImageSampler, SAttrUV * resolutionFov.xy).xyz;
                if (length(SAttrY) <-1.5) continue;
                vec3 SAttrZ = texture(normalBufferSampler, SAttrUV).xyz;
                vec3 SVPPos = texture(positionBufferSampler, SAttrUV).xyz;
                float NorDot = dot(VNormal, SVPPos - VPPos) - EPSILON;
                float TanDist = length(SVPPos-VPPos-NorDot*VNormal);
                float Angle1f = atan(NorDot,TanDist);
                float Angle2f = atan(NorDot-0.03*max(1.,StepDist*0.07),TanDist);
                float Angle1 = max(0.,ceil(Angle1f/(PI*0.5)*32.));
                float Angle2 = max(0.,floor(Angle2f/(PI*0.5)*32.));
                int SBitMask = (int(pow(2.,Angle1-Angle2))-1) << int(Angle2);
                vec3 SNormal = SAttrZ *2.-1.;
                SNormal = vec3(dot(SNormal,Tan),dot(SNormal,Bit),dot(SNormal,eyePos.xyz));

                result.xyz += float(CountBits(SBitMask & (~BitMask)))/max(1.,Angle1-Angle2)*SAttrY *LightCoeff
                    *(pow(cos(Angle2*I64*PI),2.)-pow(cos(Angle1*I64*PI),2.))
                    *sqrt(max(0.,dot(SNormal,-normalize(SVPPos-VPPos))));

                //Update bitmask
                BitMask = BitMask | SBitMask;

            }
        }
    }

    vec3 final = (result.xyz * 0.5) + (colour * 0.5);

    outColor = result;
}