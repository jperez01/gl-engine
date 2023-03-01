#version 460 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;
in vec4 FragPosLightSpace;

struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

#define NR_POINT_LIGHTS 4

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

uniform samplerCube shadowMaps[4];
uniform sampler2D shadowMap;

uniform sampler2DArray cascadedMap;
layout (std140, binding = 0) uniform LightSpaceMatrices {
    mat4 lightSpaceMatrices[16];
};
uniform float cascadePlaneDistances[16];
uniform int cascadeCount;

uniform float shininess;
uniform float far_plane;
uniform mat4 view;
uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLights[4];

float calcShadow(vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(pointLights[0].position - FragPos);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    if (projCoords.z > 1.0) shadow = 0.0;

    return shadow;
}

float shadowCubeCalculation(vec3 fragPos, int index) {
    vec3 fragToLight = fragPos - pointLights[index].position;

    float currentDepth = length(fragToLight);
    float bias = 0.05;
    float offset = 0.1f;
    float samples = 4.0f;
    float shadow = 0.0f;

    for (float x = -offset; x < offset; x += offset / (samples * 0.5)) {
        for (float y = -offset; y < offset; y += offset / (samples * 0.5)) {
            for (float z = -offset; z < offset; z += offset / (samples * 0.5)) {
                float closestDepth = texture(shadowMaps[index], fragToLight + vec3(x, y, z)).r;
                closestDepth *= far_plane;
                if (currentDepth - bias > closestDepth) shadow += 1.0;
            }
        }
    }
    shadow /= (samples * samples * samples);

    return shadow;
}

vec3 calcPointLight(PointLight light, vec3 position, vec3 normal, vec3 viewDir, int index) {
    vec3 lightDir = normalize(light.position - position);

    float diff = max(dot(normal, lightDir), 0.0);

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);

    vec3 ambient = light.ambient * texture(texture_diffuse1, TexCoords).rgb;
    vec3 specular = spec * light.specular * texture(texture_specular1, TexCoords).rgb;
    vec3 diffuse = diff * light.diffuse * texture(texture_diffuse1, TexCoords).rgb;

    float shadow = shadowCubeCalculation(FragPos, index);
    return (ambient + (1.0 - shadow) * (diffuse + specular)) * texture(texture_diffuse1, TexCoords).rgb;
}

float cascadedShadowCalculation(vec3 fragPosWorldSpace, vec3 lightDir) {
    vec4 viewSpace = view * vec4(fragPosWorldSpace, 1.0);
    float depthValue = abs(viewSpace.z);

    int layer = -1;
    for (int i = 0; i < cascadeCount; i++) {
        if (depthValue < cascadePlaneDistances[i]) {
            layer = i;
            break;
        }
    }
    if (layer == -1) layer = cascadeCount;

    vec4 lightSpacePos = lightSpaceMatrices[layer] * vec4(fragPosWorldSpace, 1.0);

    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5;

    float currentDepth = projCoords.z;
    if (currentDepth > 1.0) return 0.0;

    vec3 normal = normalize(Normal);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

    if (layer == cascadeCount) bias *= 1 / (far_plane / 0.5);
    else bias *= 1 / (cascadePlaneDistances[layer] * 0.5);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(cascadedMap, 0));

    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float pcfDepth = texture(cascadedMap, vec3(projCoords.xy + vec2(x, y) * texelSize, layer)).r;
            shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    if (projCoords.z > 1.0) shadow = 0.0;

    return shadow;
}

vec3 calcDirLight(DirLight light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(-light.direction);

    float diff = max(dot(normal, lightDir), 0.0);

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);

    vec3 ambient = light.ambient * texture(texture_diffuse1, TexCoords).rgb;
    vec3 specular = spec * light.specular * texture(texture_specular1, TexCoords).rgb;
    vec3 diffuse = diff * light.diffuse * texture(texture_diffuse1, TexCoords).rgb;

    float shadow = cascadedShadowCalculation(FragPos, lightDir);

    vec3 totalColor = ambient + (1.0 - shadow) * (diffuse + specular);
    return totalColor;
}

void main()
{    
    vec3 normal = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 result = calcDirLight(dirLight, normal, viewDir);
    for (int i = 0; i < NR_POINT_LIGHTS; i++) {
        result += calcPointLight(pointLights[i], FragPos, normal, viewDir, i);
    }

    FragColor = vec4(result, 1.0);
}