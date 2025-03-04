
struct DirectionalLight {
    vec3 Direction;
    vec3 Ambient;
    vec3 Colour;
    mat4 LightSpaceMatrix;
};

struct PointLight {
    vec4 PositionRadius;
    vec4 Ambient;
    vec4 Colour;
};

struct SpotLight {
    vec4 PositionRadius;
    vec4 DirectionAngle;
    vec4 Ambient;
    vec4 Colour;
};