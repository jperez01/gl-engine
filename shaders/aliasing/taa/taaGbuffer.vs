#version 430 core

layout(location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec4 modelViewPos;
out vec2 TexCoords;
out vec3 Normal;

out vec4 currentPos;
out vec4 previousPos;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;

uniform vec2 jitter;
uniform mat4 prevView;
uniform mat4 prevProjection;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    modelViewPos = view * worldPos;
    TexCoords = aTexCoords;
    
    mat3 normalMatrix = transpose(inverse(mat3(view * model)));
    Normal = normalMatrix * aNormal;

    vec4 clipPos = projection * view * worldPos;

    currentPos = clipPos;
    previousPos = prevProjection * prevView * worldPos;
    clipPos += vec4(jitter, 0.0, 0.0);

    gl_Position = clipPos;
}