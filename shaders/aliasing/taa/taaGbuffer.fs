#version 430 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;
layout (location = 3) out vec4 gReflectionPosition;
layout (location = 4) out vec2 gMetallicRoughness;
layout (location = 5) out vec2 gVelocity;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;
in vec4 modelViewPos;

in vec4 currentPos;
in vec4 previousPos;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_metallic1;

vec2 calculateVelocity(vec4 currentPosition, vec4 prevPosition) {
    vec2 newPos = (currentPosition.xy / currentPosition.w * 0.5) + 0.5;
    vec2 prevPos = (prevPosition.xy / prevPosition.w * 0.5) + 0.5;

    return 5.0 * (newPos - prevPos);
}

void main()
{    
    // store the fragment position vector in the first gbuffer texture
    gPosition = FragPos;
    gReflectionPosition = modelViewPos;
    // also store the per-fragment normals into the gbuffer
    gNormal = normalize(Normal);
    // and the diffuse per-fragment color
    gAlbedoSpec.rgb = texture(texture_diffuse1, TexCoords).rgb;
    // store specular intensity in gAlbedoSpec's alpha component
    gAlbedoSpec.a = texture(texture_specular1, TexCoords).r;

    gMetallicRoughness = texture(texture_metallic1, TexCoords).gb;

    gVelocity = calculateVelocity(currentPos, previousPos);
}