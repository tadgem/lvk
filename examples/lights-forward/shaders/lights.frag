#version 450

#define MAX_NUM_EACH_LIGHTS 16

#define ATTENUATION_CONSTANT 1.0
#define ATTENUATION_LINEAR_CONSTANT 4.5
#define ATTENUATION_QUADRATIC_CONSTANT 75.0

struct DirectionalLight
{
    vec4 Direction;
    vec4 Ambient;
    vec4 Colour;
};

struct PointLight
{
    vec4 PositionRadius;
    vec4 Ambient;
    vec4 Colour;
};

struct SpotLight
{
    vec4 PositionRadius;
    vec4 DirectionAngle;
    vec4 Ambient;
    vec4 Colour;
};

layout(location = 0) in vec3 Normal;
layout(location = 1) in vec2 UV;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

layout(binding = 2) uniform UniformLightObject{
    DirectionalLight    u_DirectionalLight;
    PointLight          u_PointLights[MAX_NUM_EACH_LIGHTS];
    SpotLight           u_SpotLights[MAX_NUM_EACH_LIGHTS];

    uint                u_DirLightActive;
    uint                u_PointLightsActive;
    uint                u_SpotLightsActive;
} lightUbo;

float BlinnPhong_Attenuation(float range, float dist)
{
    float l     = ATTENUATION_LINEAR_CONSTANT / range;
    float q     = ATTENUATION_QUADRATIC_CONSTANT / range;

    return 1.0 / (ATTENUATION_CONSTANT + l * dist + q * (dist * dist));
}

vec3 BlinnPhong_Directional(vec3 normal, vec3 materialAmbient, vec3 materialDiffuse, vec3 materialSpecular, float shininess)
{
    vec3 lightDir = normalize(-lightUbo.u_DirectionalLight.Direction.xyz);
    float diff = max(dot(normal, lightDir), 0.0);
    float s = pow(max(dot(normal, lightDir), 0.0), shininess);

    vec3    ambient     = lightUbo.u_DirectionalLight.Ambient.xyz * materialAmbient;
    vec3    diffuse     = lightUbo.u_DirectionalLight.Colour.xyz * diff * materialDiffuse;
	vec3    spec        = lightUbo.u_DirectionalLight.Colour.xyz * s * materialSpecular;

    return (ambient + diffuse + spec);
}

void main() {

    vec4 textureColour = vec4(texture(texSampler, UV).rgb, 1.0f);
    vec3 lightColour = BlinnPhong_Directional(Normal, vec3(0.0f), textureColour.xyz, vec3(0.0f), 0.0f);

    outColor = vec4(lightColour, 1.0);
}