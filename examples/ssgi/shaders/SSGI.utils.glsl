#define I16 1.0/16.0
#define I32 1./32.
#define I64 1./64.
#define I300 1./300.
#define I512 1./512.
#define I1024 1./1024.
#define PI 3.141592653
#define HPI (0.5*3.141592653)
#define LightCoeff 10.0
#define ILightCoeff 1.0/LightCoeff
#define EPSILON 0.001;
vec3 SampleSky(vec3 d) {
    //Returns the sky color
    vec3 SL = mix(vec3(0.2, 0.5, 1.), vec3(1., 0.2, 0.025), d.x*0.5+0.5)*((1.-0.5*d.y)*0.5);
    return SL;
}

vec3 SchlickFresnel(vec3 r0, float angle) {
    //Return a Schlick Fresnel approximation
    return r0+(1.-r0)*pow(1.-angle, 5.);
}

mat3 TBN(vec3 N) {
    //Returns the simple tangent space matrix
    vec3 Nb, Nt;
    if (abs(N.y)>0.999) {
        Nb = vec3(1., 0., 0.);
        Nt = vec3(0., 0., 1.);
    } else {
        Nb = normalize(cross(N, vec3(0., 1., 0.)));
        Nt = normalize(cross(Nb, N));
    }
    return mat3(Nb.x, Nt.x, N.x, Nb.y, Nt.y, N.y, Nb.z, Nt.z, N.z);
}

vec3 TBN(vec3 N, out vec3 O) {
    //Returns the simple tangent space directions
    if (abs(N.y)>0.999) {
        O = vec3(1., 0., 0.);
        return vec3(0., 0., 1.);
    } else {
        O = normalize(cross(N, vec3(0., 1., 0.)));
        return normalize(cross(O, N));
    }
}

vec3 RandSampleCos(vec2 v) {
    //Returns a random cos-distributed sample
    float theta = sqrt(v.x);
    float phi = 2.*3.14159*v.y;
    float x = theta*cos(phi);
    float z = theta*sin(phi);
    return vec3(x, z, sqrt(max(0., 1.-v.x)));
}

vec3 FloatToVec3(float v) {
    //Returns vec3 from int
    int VPInt = floatBitsToInt(v);
    int VPInt1024 = VPInt%1024;
    int VPInt10241024 = ((VPInt-VPInt1024)/1024)%1024;
    return vec3(VPInt1024, VPInt10241024, ((VPInt-VPInt1024-VPInt10241024)/1048576))*I1024;
}

float Vec3ToFloat(vec3 v) {
    //Returns "int" from vec3 (10 bit per channel)
    ivec3 intv = min(ivec3(floor(v*1024.)), ivec3(1023));
    return intBitsToFloat(intv.x+intv.y*1024+intv.z*1048576);
}

vec2 FloatToVec2(float v) {
    //Returns vec3 from int
    int VPInt = floatBitsToInt(v);
    int VPInt16k = VPInt%1048576;
    return vec2(VPInt16k, ((VPInt-VPInt16k)/1048576)%1048576);
}

float Vec2ToFloat(vec2 v) {
    //Returns "int" from vec3 (10 bit per channel)
    ivec2 intv = min(ivec2(floor(v*1048576.)), ivec2(1048575));
    return intBitsToFloat(intv.x+intv.y*1048576);
}

float ARand21(vec2 uv) {
    //Returns 1D noise from 2D
    return fract(sin(uv.x*uv.y)*403.125+cos(dot(uv, vec2(13.18273, 51.2134)))*173.137);
}

float ARand31(vec3 uv) {
    //Returns 1D noise from 3D
    return fract(sin(uv.x*uv.y+uv.z*uv.y*uv.x)*403.125+cos(dot(uv, vec3(33.1256, 13.18273, 51.2134)))*173.137);
}

vec3 ARand23(vec2 uv) {
    //Analytic 3D noise from 2D
    return fract(sin(uv.x*uv.y)*vec3(403.125, 486.125, 513.432)+cos(dot(uv, vec2(13.18273, 51.2134)))*vec3(173.137, 261.23, 203.127));
}

float DFBox(vec3 p, vec3 b) {
    vec3 d = abs(p-b*0.5)-b*0.5;
    return min(max(d.x, max(d.y, d.z)), 0.)+length(max(d, 0.));
}

float DFBoxC(vec3 p, vec3 b) {
    vec3 d = abs(p)-b;
    return min(max(d.x, max(d.y, d.z)), 0.)+length(max(d, 0.));
}

float DFBox(vec2 p, vec2 b) {
    vec2 d = abs(p-b*0.5)-b*0.5;
    return min(max(d.x, d.y), 0.)+length(max(d, 0.));
}

float DFBoxC(vec2 p, vec2 b) {
    vec2 d = abs(p)-b;
    return min(max(d.x, d.y), 0.)+length(max(d, 0.));
}

float DFDisk(vec3 p) {
    float d = length(p.xz-0.5)-0.35;
    vec2 w = vec2(d, abs(p.y));
    return min(max(w.x, w.y), 0.)+length(max(w, 0.));
}