#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

uniform sampler2D floorTexture;
uniform vec3 lightPos;
uniform vec3 viewPos;

void main() {
    vec3 color = texture(floorTexture, fs_in.TexCoords).rgb;

    vec3 ambient = 0.05 * color;

    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
    vec3 normal = normalize(fs_in.Normal);

    float diffMult = dot(lightDir, normal);
    vec3 diffuse = diffMult * color;

    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 halfway = normalize(viewDir + lightDir);

    float specMult = pow(max(dot(halfway, normal), 0.0), 32.0);
    vec3 specular = vec3(0.3) * specMult;

    FragColor = vec4(ambient + specular + diffuse, 1.0);
}