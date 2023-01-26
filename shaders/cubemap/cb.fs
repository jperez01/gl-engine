#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 Position;

uniform vec3 cameraPos;
uniform samplerCube skybox;

void main() {
    float ratio = 1.00 / 1.309;
    vec3 I = normalize(Position - cameraPos);
    vec3 ref = reflect(I, normalize(Normal));
    // ref = refract(I, normalize(Normal), ratio);
    vec4 inter = vec4(texture(skybox, ref).rgb, 1.0);
    FragColor = inter;
}