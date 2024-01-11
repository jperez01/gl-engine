#version 430 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler3D worleyNoiseTexture;
uniform float layer;

void main() {
	vec3 finalTexCoords = vec3(TexCoords, layer);
	vec3 textureColor = textureLod(worleyNoiseTexture, finalTexCoords, 0.0).rgb;
	vec4 finalValue = vec4(textureColor, 1.0);

	FragColor = finalValue;
}