#version 300 es
precision highp float;

#define MAX_LIGHTS              8
#define LIGHT_DIRECTIONAL       0
#define LIGHT_POINT             1
#define LIGHT_SPOT              2
#define PI 3.14159265358979323846

struct Light {
    int enabled;
    int type;
    vec3 position;
    vec3 direction;
    vec4 color;
    float intensity;
    float range;
    float falloff;
    float cosInner;
    float cosOuter;
};

// Input vertex attributes (from vertex shader)
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;
in vec4 shadowPos;
in vec4 shadowPosSpot;
in mat3 TBN;

// Output fragment color
out vec4 finalColor;

// Input uniform values
uniform int numOfLights;
uniform sampler2D albedoMap;
uniform sampler2D mraMap;
uniform sampler2D normalMap;
uniform sampler2D emissiveMap; // r: Hight g:emissive

uniform vec2 tiling;
uniform vec2 offset;

uniform int useTexAlbedo;
uniform int useTexNormal;
uniform int useTexMRA;
uniform int useTexEmissive;

uniform vec4  albedoColor;
uniform vec4  emissiveColor;
uniform float normalValue;
uniform float metallicValue;
uniform float roughnessValue;
uniform float aoValue;
uniform float emissivePower;

// Input lighting values
uniform Light lights[MAX_LIGHTS];
uniform vec3 viewPos;

uniform vec3 ambientColor;
uniform float ambient;

uniform sampler2D shadowMap;
uniform float shadowBias;

uniform sampler2D shadowMapSpot;
uniform float shadowBiasSpot;

// distance attenuation for point/spot lights
float Attenuation(float dist, float range, float falloff) {
    float s = clamp(dist / range, 0.0, 1.0);
    return pow(1.0 - s, falloff);
}

// angular falloff for spot lights
float SpotFactor(vec3 L, vec3 dir, float cosInner, float cosOuter) {
    float cosTheta = dot(-L, dir);
    return clamp((cosTheta - cosOuter) / (cosInner - cosOuter), 0.0, 1.0);
}

// Reflectivity in range 0.0 to 1.0
// NOTE: Reflectivity is increased when surface view at larger angle
vec3 SchlickFresnel(float hDotV,vec3 refl)
{
    return refl + (1.0 - refl)*pow(1.0 - hDotV, 5.0);
}

float GgxDistribution(float nDotH,float roughness)
{
    float a = roughness * roughness * roughness * roughness;
    float d = nDotH * nDotH * (a - 1.0) + 1.0;
    d = PI * d * d;
    return a / max(d,0.0000001);
}

float GeomSmith(float nDotV,float nDotL,float roughness)
{
    float r = roughness + 1.0;
    float k = r*r / 8.0;
    float ik = 1.0 - k;
    float ggx1 = nDotV/(nDotV*ik + k);
    float ggx2 = nDotL/(nDotL*ik + k);
    return ggx1*ggx2;
}

float ShadowCalc(vec4 sp, sampler2D smap, float bias) {
    vec3 proj = sp.xyz / sp.w;
    proj = proj * 0.5 + 0.5;
    if (proj.z > 1.0) return 1.0;
    // 3x3 PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(smap, 0));
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float d = texture(smap, proj.xy + vec2(x, y) * texelSize).r;
            shadow += (proj.z - bias > d) ? 0.0 : 1.0;
        }
    }
    return shadow / 9.0;
}

vec3 ComputePBR()
{
    vec3 albedo = texture(albedoMap,vec2(fragTexCoord.x*tiling.x + offset.x, fragTexCoord.y*tiling.y + offset.y)).rgb;
    albedo = vec3(albedoColor.x*albedo.x, albedoColor.y*albedo.y, albedoColor.z*albedo.z);

    float metallic = clamp(metallicValue, 0.0, 1.0);
    float roughness = clamp(roughnessValue, 0.0, 1.0);
    float ao = clamp(aoValue, 0.0, 1.0);

    if (useTexMRA == 1)
    {
        vec4 mra = texture(mraMap, vec2(fragTexCoord.x*tiling.x + offset.x, fragTexCoord.y*tiling.y + offset.y))*float(useTexMRA);
        metallic = clamp(mra.r + metallicValue, 0.04, 1.0);
        roughness = clamp(mra.g + roughnessValue, 0.04, 1.0);
        ao = (mra.b + aoValue)*0.5;
    }

    vec3 N = normalize(fragNormal);
    if (useTexNormal == 1)
    {
        N = texture(normalMap, vec2(fragTexCoord.x*tiling.x + offset.y, fragTexCoord.y*tiling.y + offset.y)).rgb;
        N = normalize(N*2.0 - 1.0);
        N = normalize(N*TBN);
    }

    vec3 V = normalize(viewPos - fragPosition);

    vec3 emissive = vec3(0);
    emissive = (texture(emissiveMap, vec2(fragTexCoord.x*tiling.x+offset.x, fragTexCoord.y*tiling.y+offset.y)).rgb).g * emissiveColor.rgb*emissivePower * float(useTexEmissive);

    // if dia-electric use base reflectivity of 0.04 otherwise ut is a metal use albedo as base reflectivity
    vec3 baseRefl = mix(vec3(0.04), albedo.rgb, metallic);
    vec3 lightAccum = vec3(0.0);

    for (int i = 0; i < numOfLights; i++)
    {
        if (lights[i].enabled == 0) continue;

        vec3 L;
        float attenuation;

        if (lights[i].type == LIGHT_DIRECTIONAL) {
            L = -normalize(lights[i].direction);
            attenuation = 1.0;
        } else {
            // point or spot
            vec3 toLight = lights[i].position - fragPosition;
            float dist = length(toLight);
            L = toLight / dist;
            attenuation = Attenuation(dist, lights[i].range, lights[i].falloff);

            if (lights[i].type == LIGHT_SPOT) {
                attenuation *= SpotFactor(L, normalize(lights[i].direction),
                                          lights[i].cosInner, lights[i].cosOuter);
            }
        }

        vec3 H = normalize(V + L);
        vec3 radiance = lights[i].color.rgb * lights[i].intensity * attenuation;

        // Cook-Torrance BRDF distribution function
        float nDotV = max(dot(N,V), 0.0000001);
        float nDotL = max(dot(N,L), 0.0000001);
        float hDotV = max(dot(H,V), 0.0);
        float nDotH = max(dot(N,H), 0.0);
        float D = GgxDistribution(nDotH, roughness);
        float G = GeomSmith(nDotV, nDotL, roughness);
        vec3 F = SchlickFresnel(hDotV, baseRefl);

        vec3 spec = (D*G*F)/(4.0*nDotV*nDotL);

        vec3 kD = vec3(1.0) - F;
        kD *= 1.0 - metallic;
        float shadow = 1.0;
        if (lights[i].type == LIGHT_DIRECTIONAL) shadow = ShadowCalc(shadowPos, shadowMap, shadowBias);
        else if (lights[i].type == LIGHT_SPOT) shadow = ShadowCalc(shadowPosSpot, shadowMapSpot, shadowBiasSpot);
        lightAccum += (kD*albedo.rgb/PI + spec)*radiance*nDotL * shadow;
    }

    vec3 ambientFinal = (ambientColor + albedo)*ambient*0.5;

    return ambientFinal + lightAccum*ao + emissive;
}

void main()
{
    vec3 color = ComputePBR();

    // HDR tonemapping
    color = pow(color, color + vec3(1.0));

    // Gamma correction
    color = pow(color, vec3(1.0/2.2));

    finalColor = vec4(color, 1.0);
}
