#version 430 core
layout (location = 0) out vec4 historyColor;

in vec2 TexCoords;

uniform sampler2D colorTexture;

void main() {
    historyColor = texture(colorTexture, TexCoords); 
}