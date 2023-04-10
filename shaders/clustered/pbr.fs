#version 430 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;
in mat3 TBN;

struct PointLight {
    vec4 position;
    vec4 color;
    uint enabled;
    float intensity;
    float range;
    float something;
};

struct DirLight {
    vec3 direction;
    vec3 color;
};

struct LightGrid {
    uint offset;
    uint count;
};

layout (std430, binding = 2) buffer screenToView {
    mat4 inverseProj;
    uvec4 tileSizes;
    uvec2 screenDimensions;
    uvec2 tileScreenSizes;
};

layout (std430, binding = 3) buffer lightSSBO {
    PointLight pointLight[];
};

layout (std430, binding = 4) buffer lightIndexSSBO {
    uint globalLightIndexList[];
};

layout (std430, binding = 5) buffer lightGridSSBO {
    LightGrid lightGrid[];
};

#define M_PI 3.1415926535897932384626433832795

uniform float zNear;
uniform float zFar;
uniform float scale;
uniform float bias;
uniform float multiplier;

uniform vec3 viewPos;
uniform mat4 view;
uniform mat4 projection;

uniform DirLight dirLight;

uniform sampler2D texture_diffuse;
uniform sampler2D texture_specular;
uniform sampler2D texture_normal;
uniform sampler2D texture_metallic;

uniform bool noNormalMap;
uniform bool useFragNormalFunction;
uniform bool noMetallicMap;

vec3 getNormalFromMap();
vec3 calcPointLight(uint index, vec3 position, vec3 normal, 
    vec3 viewDir, vec3 albedo, float roughness, 
    float metallic, vec3 F0, float viewDistance);
vec3 calcDirLight(vec3 normal, vec3 viewDir, vec3 albedo, 
    float roughness, float metallic, vec3 F0);
float linearDepth(float depthSample);

vec3 fresnelSchlick(float cosTheta, vec3 F0);
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness);
float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);

void main()
{             
    // retrieve data from gbuffer
    vec3 albedo = texture(texture_diffuse, TexCoords).rgb;
    float metallic;
    float roughness;
    if (noMetallicMap) {
        metallic = 0.2;
        roughness = 0.8;
    } else {
        metallic = texture(texture_metallic, TexCoords).b;
        roughness = texture(texture_metallic, TexCoords).g;
    }

    vec3 normal;
    if (noNormalMap) normal = normalize(Normal);
    else  {
        if (useFragNormalFunction) {
            normal = getNormalFromMap();
        } else {
            vec3 tangentNormal = texture(texture_normal, TexCoords).xyz * 2.0 - 1.0;
            normal = normalize(TBN * tangentNormal);
        }
    }

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-viewDir, normal);
    float viewDistance = length(viewPos - FragPos);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    uint zTile = uint(max(log(linearDepth(gl_FragCoord.z)) * scale + bias, 0.0));
    uvec3 tiles = uvec3(uvec2(gl_FragCoord.xy / tileScreenSizes), zTile);
    uint tileIndex = tiles.x + 
        (tileSizes.x * tiles.y) + 
        (tileSizes.x * tileSizes.y) * tiles.z;

    uint lightCount = lightGrid[tileIndex].count;
    uint lightIndexOffset = lightGrid[tileIndex].offset;

    vec3 radianceOut = calcDirLight(normal, viewDir, albedo, roughness, metallic, F0);
    for (uint i = 0; i < lightCount; i++) {
        uint lightIndex = globalLightIndexList[lightIndexOffset + i];
        radianceOut += calcPointLight(lightIndex, FragPos, normal, 
            viewDir, albedo, roughness, 
            metallic, F0, viewDistance);
    }
    vec3 ambient = vec3(0.125) * albedo;
    radianceOut += ambient;

    FragColor = vec4(radianceOut, 1.0);
}

vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(texture_normal, TexCoords).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(FragPos);
    vec3 Q2  = dFdy(FragPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);

    vec3 N   = normalize(Normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

vec3 calcPointLight(uint index, vec3 position, vec3 normal, 
    vec3 viewDir, vec3 albedo, float roughness, 
    float metallic, vec3 F0, float viewDistance) {
    PointLight light = pointLight[index];

    float radius = light.range;
    vec3 color = light.color.rgb;

    vec3 lightDir = normalize(light.position.xyz - position);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float nDotV = max(dot(normal, viewDir), 0.0);
    float nDotL = max(dot(normal, lightDir), 0.0);

    float distance = length(light.position.xyz - position);
    float attenuation = pow(clamp(1 - pow((distance / radius), 4.0), 0.0, 1.0), 2.0)/(1.0  + (distance * distance) );
    vec3 radianceIn = color * attenuation;

    float NDF = DistributionGGX(normal, halfwayDir, roughness);
    float G = GeometrySmith(normal, viewDir, lightDir, roughness);
    vec3 F = fresnelSchlick(max(dot(halfwayDir, viewDir), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * nDotV * nDotL;
    vec3 specular = numerator / max(denominator, 0.000001);

    vec3 radiance = (kD * albedo / M_PI + specular) * radianceIn * nDotL;
    return multiplier * radiance;
}

vec3 calcDirLight(vec3 normal, vec3 viewDir, vec3 albedo, float roughness, float metallic, vec3 F0) {
    vec3 lightDir = normalize(-dirLight.direction);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    float nDotL = max(dot(normal, lightDir), 0.0);
    float nDotV = max(dot(normal, viewDir), 0.0);

    vec3 radianceIn = dirLight.color;

    float NDF = DistributionGGX(normal, halfwayDir, roughness);
    float G = GeometrySmith(normal, viewDir, lightDir, roughness);
    vec3 F = fresnelSchlick(max(dot(halfwayDir, viewDir), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * nDotV * nDotL;
    vec3 specular = numerator / max(denominator, 0.000001);

    vec3 radiance = (kD * albedo / M_PI + specular) * radianceIn * nDotL;
    return radiance;
}

float linearDepth(float depthSample) {
    float depthRange = 2.0 * depthSample - 1.0;

    float linear = 2.0 * zNear * zFar / (zFar + zNear - depthRange * (zFar - zNear));
    return linear;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = M_PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}