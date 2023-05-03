#version 460 core
#extension GL_ARB_bindless_texture : require

out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;
flat in int drawID;

struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

layout(binding = 0) buffer textureHandleBlock {
    uvec2 texture_handles[409];
};

layout(binding = 1) buffer textureLengthBlock {
    uint textureLengths[];
};

uniform int index;
uniform float shininess;
uniform vec3 viewPos;
uniform DirLight dirLight;

vec3 calcDirLight(DirLight light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(-light.direction);

    float diff = max(dot(normal, lightDir), 0.0);

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);

    uint numTextures = textureLengths[drawID];
    uvec2 handle = texture_handles[numTextures];
    sampler2D diffuseSampler = sampler2D(handle);

    vec4 albedo = texture(diffuseSampler, TexCoords);

    vec3 ambient = light.ambient * albedo.rgb;
    vec3 specular = spec * light.specular * albedo.a;
    vec3 diffuse = diff * light.diffuse * albedo.rgb;

    return ambient + specular + diffuse;
}

void main()
{    
    vec3 normal = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 result = calcDirLight(dirLight, normal, viewDir);

    vec3 value = vec3(drawID * 0.1f);

    FragColor = vec4(result, 1.0);
}