#version 460 core

in vec3 FragPos;
in vec3 FragNormal;
in vec2 FragTexCoords;
in vec3 WorldVoxelPos;

struct DirLight {
    vec3 direction;

    vec3 color;
};

struct PointLight {
    vec3 position;

    vec3 color;
};

#define NR_LIGHTS 4

layout (RGBA8) uniform image3D voxelTexture;

uniform sampler2D texture_diffuse;
uniform sampler2D texture_normal;

uniform DirLight dirLight;
uniform PointLight pointLights[NR_LIGHTS];

uniform mat4 view;
uniform mat4 projection;
uniform int gridSize;


vec3 calcPointLight(PointLight light, vec3 position, vec3 normal);
vec3 calcDirLight(DirLight light, vec3 normal);

void main() {
    vec3 normal = normalize(FragNormal);

    vec3 totalColor = calcDirLight(dirLight, normal);
    for (int i = 0; i < NR_LIGHTS; i++) {
        totalColor += calcPointLight(pointLights[i], FragPos, FragNormal);
    }

    vec3 someColor = texture(texture_diffuse, FragTexCoords).xyz;

    imageStore(voxelTexture, ivec3(WorldVoxelPos), vec4(someColor, 1.0));
}

vec3 calcPointLight(PointLight light, vec3 position, vec3 normal) {
    vec3 lightDir = normalize(light.position - position);

    float diff = max(dot(normal, lightDir), 0.0);

    vec3 ambient = light.color * texture(texture_diffuse, FragTexCoords).rgb;
    vec3 diffuse = diff * light.color * texture(texture_diffuse, FragTexCoords).rgb;

    return ambient + diffuse;
}

vec3 calcDirLight(DirLight light, vec3 normal) {
    vec3 lightDir = normalize(-light.direction);

    float diff = max(dot(normal, lightDir), 0.0);

    vec3 ambient = light.color * texture(texture_diffuse, FragTexCoords).rgb;
    vec3 diffuse = diff * light.color * texture(texture_diffuse, FragTexCoords).rgb;

    return ambient + diffuse;
}