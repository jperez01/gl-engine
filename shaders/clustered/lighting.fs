#version 430 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

struct PointLight {
    vec4 position;
    vec4 color;
    uint enabled;
    float intensity;
    float range;
    float something;
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

uniform vec3 viewPos;
uniform float zNear;
uniform float zFar;
uniform float scale;
uniform float bias;
uniform float multiplier;

uniform mat4 view;
uniform mat4 projection;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

vec3 colors[8] = vec3[](
   vec3(0, 0, 0),    vec3( 0,  0,  1), vec3( 0, 1, 0),  vec3(0, 1,  1),
   vec3(1,  0,  0),  vec3( 1,  0,  1), vec3( 1, 1, 0),  vec3(1, 1, 1)
);

vec3 calcPointLight(uint index, vec3 position, vec3 normal, vec3 viewDir, vec3 diffColor, float specFactor) {
    PointLight light = pointLight[index];

    float radius = light.range;
    vec3 lightDir = normalize(light.position.xyz - position);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    float nDotV = max(dot(normal, viewDir), 0.0);
    float nDotL = max(dot(normal, lightDir), 0.0);

    float distance = length(light.position.xyz - position);
    float attenuation = pow(clamp(1 - pow((distance / radius), 4.0), 0.0, 1.0), 2.0) / (1.0 + (distance * distance));

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 100);

    vec3 ambient = light.color.xyz * diffColor;
    vec3 specular = spec * light.color.xyz * texture(texture_specular1, TexCoords).rgb;
    vec3 diffuse = nDotL * light.color.xyz * diffColor;

    return multiplier * (specular + diffuse);
}

float linearDepth(float depthSample){
    float depthRange = 2.0 * depthSample - 1.0;

    float linear = 2.0 * zNear * zFar / (zFar + zNear - depthRange * (zFar - zNear));
    return linear;
}

void main()
{             
    // retrieve data from gbuffer
    vec3 normal = normalize(Normal);
    vec3 Diffuse = texture(texture_diffuse1, TexCoords).rgb;
    float Specular = texture(texture_specular1, TexCoords).a;
    vec3 viewDir = normalize(viewPos - FragPos);

    uint zTile = uint(max(log(linearDepth(gl_FragCoord.z)) * scale + bias, 0.0));
    uvec3 tiles = uvec3(uvec2(gl_FragCoord.xy / tileScreenSizes), zTile);
    uint tileIndex = tiles.x + 
        (tileSizes.x * tiles.y) + 
        (tileSizes.x * tileSizes.y) * tiles.z;

    uint lightCount = lightGrid[tileIndex].count;
    uint lightIndexOffset = lightGrid[tileIndex].offset;

    vec3 lighting  = Diffuse * 0.5;
    for (uint i = 0; i < lightCount; i++) {
        uint lightIndex = globalLightIndexList[lightIndexOffset + i];
        lighting += calcPointLight(lightIndex, FragPos, normal, viewDir, Diffuse, Specular);
    }

    //FragColor = vec4(colors[uint(mod(zTile, 8))], 1.0);
    // FragColor = vec4(vec3(tileIndex) / vec3(6144) , 1.0);

    FragColor = vec4(lighting, 1.0);
}