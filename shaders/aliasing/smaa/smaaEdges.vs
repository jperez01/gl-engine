#version 430 core
layout(location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec4 Offsets[3];

uniform vec4 resolutionAndInverse;

void main() {
    TexCoords = aTexCoords;

    Offsets[0] = resolutionAndInverse.xyxy * vec4(-1.0, 0.0, 0.0, -1.0) + aTexCoords.xyxy;
    Offsets[1] = resolutionAndInverse.xyxy * vec4(1.0, 0.0, 0.0, 1.0) + aTexCoords.xyxy;
    Offsets[2] = resolutionAndInverse.xyxy * vec4(-2.0, 0.0, 0.0, -2.0) + aTexCoords.xyxy;

    gl_Position = vec4(aPos, 1.0);
}