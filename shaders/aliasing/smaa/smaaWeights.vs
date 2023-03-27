#version 430 core
layout(location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec2 PixelCoords;
out vec4 Offsets[3];

uniform vec4 resolutionAndInverse;
uniform float maxSearchSteps = 8.0;

void main() {
    TexCoords = aTexCoords;
    PixelCoords = TexCoords * resolutionAndInverse.zw;

    Offsets[0] = resolutionAndInverse.xyxy * vec4(-0.25, -0.125,  1.25, -0.125) + aTexCoords.xyxy;
    Offsets[1] = resolutionAndInverse.xyxy * vec4(-0.125, -0.25, -0.125,  1.25) + aTexCoords.xyxy;
    Offsets[2] = resolutionAndInverse.xyxy * vec4(-2.0, 2.0, -2.0, 2.0) * maxSearchSteps
         + vec4(Offsets[0].xz, Offsets[1].yw);

    gl_Position = vec4(aPos, 1.0);
}
