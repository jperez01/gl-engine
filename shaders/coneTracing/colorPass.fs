#version 430 core

out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;
in vec3 Tangent;
in vec4 FragPosLightSpace;
in mat3 TBN;

struct DirLight {
    vec3 direction;

    vec3 color;
};

struct PointLight {
    vec3 position;

    vec3 color;
};

#define NR_LIGHTS 4

const float PI = 3.14159265359;
const float HALF_PI = 1.57079f;
const float ALPHA_THRESH = 0.5;

uniform sampler2D texture_diffuse;
uniform sampler2D texture_normal;
uniform sampler2D texture_metallic;
uniform sampler2D shadowMap;
uniform sampler3D voxelTexture;

uniform float VOXEL_SIZE;
uniform float MAX_DIST = 100.0;
uniform int coneToUse = -1;

uniform float indirectLightMultiplier = 0.5;
uniform float specularAngleMultiplier = 0.1;

uniform float voxelWorldSize;
uniform int gridSize;
uniform mat4 voxelProjection;
uniform float someLod;

uniform bool noNormal;
uniform bool noMetallicMap;

uniform float shininess;
uniform vec3 viewPos;

uniform vec3 dirLightPos;
uniform float dirLightMultiplier;
uniform DirLight dirLight;
uniform PointLight pointLights[NR_LIGHTS];

const int NUM_CONES = 6;
vec3 coneDirections[6] = vec3[]
(                            
    vec3(0, 1, 0),
    vec3(0, 0.5, 0.866025),
    vec3(0.823639, 0.5, 0.267617),
    vec3(0.509037, 0.5, -0.700629),
    vec3(-0.509037, 0.5, -0.700629),
    vec3(-0.823639, 0.5, 0.267617)
    );
float coneWeights[6] = float[](0.25, 0.15, 0.15, 0.15, 0.15, 0.15);

vec3 calcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 calcDirLight(vec3 normal, vec3 viewDir, vec3 albedo, float roughness, float metallic, vec3 F0);
float shadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir);

vec3 calcPointLight(PointLight light, vec3 position, vec3 normal, vec3 viewDir);

vec3 calcIndirectLighting(vec3 albedo, vec3 normal, vec3 tangent, float metallic);
vec3 coneTrace(vec3 direction, float aperture);
vec4 sampleVoxels(vec3 worldPosition, float lod);

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

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

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec3 tangent = normalize(Tangent);
    vec3 normal;
    if (noNormal) {
        normal = normalize(Normal);
    } else {
        vec3 tangentNormal = texture(texture_normal, TexCoords).xyz * 2.0 - 1.0;
        normal = normalize(TBN * tangentNormal);
    }

    vec2 metalRough = texture(texture_metallic, TexCoords).bg;
    float metallic, roughness;
    if (noMetallicMap) {
        metallic = 0.2;
        roughness = 0.8;
    } else {
        metallic = metalRough.r;
        roughness = metalRough.g;
    }

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 albedo = texture(texture_diffuse, TexCoords).rgb;
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 totalColor = dirLightMultiplier * calcDirLight(normal, viewDir, albedo, roughness, metallic, F0);
    for (int i = 0; i < 0; i++) {
        totalColor += calcPointLight(pointLights[i], FragPos, normal, viewDir);
    }
    totalColor += indirectLightMultiplier * calcIndirectLighting(albedo, normal, tangent, metallic);
    
    FragColor = vec4(totalColor, 1.0);
}

vec3 calcIndirectLighting(vec3 albedo, vec3 normal, vec3 tangent, float metallic) {
    vec3 color = vec3(0.0);

    vec3 bitangent = normalize(cross(tangent, normal));

    mat3 newTNB = mat3(bitangent, normal, tangent);

    if (coneToUse == -1) {
        for (int i = 0; i < NUM_CONES; i++) {
            color += coneWeights[i] * coneTrace(newTNB * coneDirections[i], 0.577);
        }
    } else {
        color = coneTrace(newTNB * coneDirections[coneToUse], 0.577);
    }

    // Calculate specular indirect
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-viewDir, normal);
    reflectDir = normalize(reflectDir);

    float specHalfAngle = clamp(tan(HALF_PI * specularAngleMultiplier), 0.0174533f, PI);
    vec3 specularTraceColor = metallic * coneTrace(reflectDir, specHalfAngle);

    return (color + specularTraceColor) * albedo;
}

vec3 coneTrace(vec3 direction, float tanHalfAngle) {
    direction = normalize(direction);

    vec3 totalColor = vec3(0.0);
    float alpha = 0.0;

    float dist = voxelWorldSize;

    vec3 startPos = FragPos + Normal * voxelWorldSize;

    while (dist < MAX_DIST && alpha < ALPHA_THRESH) {
        float diameter = 2 * dist * tanHalfAngle;
        float mipLevel = log2(diameter / voxelWorldSize);

        vec3 position = startPos + dist * direction;
        vec4 voxelColor = sampleVoxels(position, mipLevel);

        if (voxelColor.a > 0.0) {
            float a = 1.0 - alpha;
            totalColor += a * voxelColor.rgb;
            alpha += a * voxelColor.a;
        }

        // occlusion += (a * voxelColor.a) / (1.0 + 0.03 * diameter);
        dist += diameter * 0.5;
    }

    return totalColor;
}

vec4 sampleVoxels(vec3 worldPosition, float lod) {
    vec4 convertedPos = voxelProjection * vec4(worldPosition, 1.0);
    vec3 voxelTextureUV = convertedPos.xyz;
    voxelTextureUV = voxelTextureUV * 0.5 + 0.5;

    return textureLod(voxelTexture, voxelTextureUV, lod);
}

vec3 calcPointLight(PointLight light, vec3 position, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(light.position - position);

    float diff = max(dot(normal, lightDir), 0.0);

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);

    vec3 materialColor = texture(texture_diffuse, TexCoords).rgb;

    vec3 ambient = light.color * materialColor;
    vec3 specular = spec * light.color * materialColor;
    vec3 diffuse = diff * light.color * materialColor;

    return ambient + specular + diffuse;
}

vec3 calcDirLight(DirLight light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(-light.direction);

    float diff = max(dot(normal, lightDir), 0.0);

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);

    vec3 materialColor = texture(texture_diffuse, TexCoords).rgb;

    vec3 ambient = light.color * materialColor;
    vec3 specular = spec * light.color * materialColor;
    vec3 diffuse = diff * light.color * materialColor;

    float shadow = shadowCalculation(FragPosLightSpace, normal, normalize(dirLightPos - FragPos));
    return ambient + (1.0 - shadow) * (specular + diffuse);
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

    float visibility = shadowCalculation(FragPosLightSpace, normal, normalize(dirLightPos - FragPos));

    vec3 radiance = (kD * albedo / PI + specular) * radianceIn * nDotL;
    return radiance * (1.0 - visibility);
}

float shadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -2; x <= 2; x++) {
        for (int y = -2; y <= 2; y++) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 25.0;

    if (projCoords.z > 1.0) shadow = 0.0;

    return shadow;
}