#version 430 core
layout (location = 0) out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D historyBuffer;
uniform sampler2D currentColorBuffer;
uniform sampler2D velocityBuffer;

void main() {
    vec2 velocity = texture(velocityBuffer, TexCoords).xy / 5.0;
    vec2 previousPixelPos = TexCoords - velocity;

    vec3 currentColor = texture(currentColorBuffer, TexCoords).rgb;
    vec3 pastColor = texture(historyBuffer, previousPixelPos).rgb;

    vec3 nearColorTop = textureOffset(currentColorBuffer, TexCoords, ivec2(0, 1)).rgb;
    vec3 nearColorBottom = textureOffset(currentColorBuffer, TexCoords, ivec2(0, -1)).rgb;
    vec3 nearColorLeft = textureOffset(currentColorBuffer, TexCoords, ivec2(-1, 0)).rgb;
    vec3 nearColorRight = textureOffset(currentColorBuffer, TexCoords, ivec2(1, 0)).rgb;

    vec3 totalMin = min(min(nearColorTop, nearColorBottom), min(nearColorLeft, nearColorRight));
    vec3 totalMax = max(max(nearColorTop, nearColorBottom), max(nearColorLeft, nearColorRight));

    pastColor = clamp(pastColor, totalMin, totalMax);

    float modulationFactor = 0.9;

    vec3 finalColor = mix(currentColor, pastColor, modulationFactor);

    FragColor = vec4(finalColor, 1.0);
}