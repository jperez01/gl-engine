#version 430 core

layout (location = 0) in vec3 aPos;

out vec3 WorldPos;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main() {
	vec3 convertedPos = (aPos + 1.0) * 0.5;
	WorldPos = vec3(model * vec4(convertedPos, 1.0));
	gl_Position = projection * view * model * vec4(aPos, 1.0);
}