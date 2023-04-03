#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 WorldPos;
out vec3 Normal;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    TexCoords = aTexCoords;
    vec4 somePos = model * vec4(aPos, 1.0);
    WorldPos = somePos.xyz;
    Normal = (transpose(inverse(model)) * vec4(aNormal, 1.0)).rgb; 

    gl_Position =  projection * view * somePos;
}