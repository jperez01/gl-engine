#version 430 core

out vec4 FragColor;

in vec2 TexCoords;
in vec4 Offsets[3];

uniform sampler2D colorTexture;

const vec2 threshold = vec2(0.1);
const float adaptationFactor = 2.0;

float rgb2luma(vec3 rgb) {
    return dot(rgb, vec3(0.2126, 0.7152, 0.0722));
}

void main() {
    float lumaCenter = rgb2luma(texture2D(colorTexture, TexCoords).rgb);

    float lumaLeft = rgb2luma(texture2D(colorTexture, Offsets[0].xy).rgb);
    float lumaTop = rgb2luma(texture2D(colorTexture, Offsets[0].zw).rgb);

    vec4 delta;
    delta.xy = abs(lumaCenter - vec2(lumaLeft, lumaTop));
    vec2 edges = step(threshold, delta.xy);

    if (dot(edges, vec2(1.0)) == 0.0) discard;

    float lumaRight = rgb2luma(texture2D(colorTexture, Offsets[1].xy).rgb);
    float lumaBottom = rgb2luma(texture2D(colorTexture, Offsets[1].zw).rgb);
    delta.zw = abs(lumaCenter - vec2(lumaRight, lumaBottom));

    vec2 maxDelta = max(delta.xy, delta.zw);

    float lumaLeftLeft = rgb2luma(texture2D(colorTexture, Offsets[2].xy).rgb);
    float lumaTopTop = rgb2luma(texture2D(colorTexture, Offsets[2].zw).rgb);
    delta.zw = abs(vec2(lumaLeft, lumaTop) - vec2(lumaLeftLeft, lumaTopTop));

    maxDelta = max(maxDelta.xy, delta.zw);
    float finalDelta = max(maxDelta.x, maxDelta.y);

    edges.xy *= step(finalDelta, delta.xy * adaptationFactor);

    FragColor = vec4(edges, 0.0, 1.0);
}