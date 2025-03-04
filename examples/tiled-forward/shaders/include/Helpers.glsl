
vec3 ReconstructPositionFromDepth(float depth, vec2 uv, mat4 inverseView, mat4 inverseProjection)
{
    return vec3(0.0);
}

vec3 UnpackNormalMap( vec3 TextureSample )
{
    vec2 NormalXY = TextureSample.rg;
    NormalXY = NormalXY * vec2(2.0,2.0) - vec2(1.0,1.0);
    float NormalZ = sqrt( clamp(( 1.0f - dot( NormalXY, NormalXY ) ),0.0,1.0));
    return vec3( NormalXY.xy, NormalZ);
}

vec3 GetNormalFromMap(vec3 position, vec2 uv, vec3 normal, vec4 normal_map_sample) {
    vec3 tangentNormal = normal_map_sample.xyz * 2.0 - 1.0;

    if(abs(tangentNormal.z) < 0.0001) {
        tangentNormal = UnpackNormalMap(tangentNormal);
    }

    vec3 Q1 = dFdx(position);
    vec3 Q2 = dFdy(position);

    vec2 st1 = dFdx(uv);
    vec2 st2 = dFdy(uv);

    vec3 N = normalize(normal);
    vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
    vec3 B = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}