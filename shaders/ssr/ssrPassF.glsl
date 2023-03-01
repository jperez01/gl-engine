#version 460 core

layout (location = 0) out vec4 FragColor;

uniform sampler2D positionTexture;
uniform sampler2D normalTexture;
uniform sampler2D colorTexture;

uniform mat4 projection;
uniform mat4 invProjection;
uniform mat4 view;
uniform mat4 invView;

void main() {
    float maxDistance = 10;
    float resolution = 0.3;
    int steps = 10;
    float thickness = 0.95;

    vec2 texSize = textureSize(positionTexture, 0).xy;
    vec2 texCoord = gl_FragCoord.xy / texSize;

    vec4 uv = vec4(0.0);

    vec4 positionFrom = texture(positionTexture, texCoord);

    if (positionFrom.w <= 0.0) {
        FragColor = uv;
        return;
    }

    vec3 unitPositionFrom = normalize(positionFrom.xyz);
    vec3 normal = vec3(texture(normalTexture, texCoord));
    vec3 pivot = normalize(reflect(unitPositionFrom, normal));

    vec4 positionTo = positionFrom;

    vec4 startView = vec4(positionFrom.xyz , 1.0);
    vec4 endView = vec4(positionFrom.xyz + (pivot * maxDistance), 1.0);

    vec4 startFrag = projection * startView;
    startFrag.xyz /= startFrag.w;
    startFrag.xy = startFrag.xy * 0.5 + 0.5;
    startFrag.xy *= texSize;

    vec4 endFrag = projection * endView;
    endFrag.xyz /= endFrag.w;
    endFrag.xy = endFrag.xy * 0.5 + 0.5;
    endFrag.xy *= texSize;

    vec2 frag = startFrag.xy;
    uv.xy = frag / texSize;

    float deltaX = endFrag.x - startFrag.x;
    float deltaY = endFrag.y - startFrag.y;

    float useX = abs(deltaX) >= abs(deltaY) ? 1 : 0;
    float delta = mix(abs(deltaY), abs(deltaX), useX) * clamp(resolution, 0, 1);
    vec2 increment = vec2(deltaX, deltaY) / max(delta, 0.001);

    float viewDistance = startView.y;
    float depth = thickness;

    float search0 = 0, search1 = 0;
    int hit0 = 0, hit1 = 0;

    for (int i = 0; i < int(delta); i++) {
        frag += increment;
        uv.xy = frag / texSize;
        positionTo = texture(positionTexture, uv.xy);

        search1 = mix((frag.y - startFrag.y) / deltaY, (frag.x - startFrag.x) / deltaX, useX);
        search1 = clamp(search1, 0.0, 1.0);

        viewDistance = (startView.y * endView.y) / mix(endView.y, startView.y, search1);
        depth = viewDistance - positionTo.y;

        if (depth > 0 && depth < thickness) {
            hit0 = 1;
            break;
        } else search0 = search1;
    }

    search1 = search0 + (search1 - search0) / 2;
    steps *= hit0;

    for (int i = 0; i < steps; i++) {
        frag = mix(startFrag.xy, endFrag.xy, search1);
        uv.xy = frag / texSize;
        positionTo = texture(positionTexture, uv.xy);

        viewDistance = (startView.y * endView.y) / mix(endView.y, startView.y, search1);
        depth = viewDistance - positionTo.y;

        if (depth > 0 && depth < thickness) {
            hit1 = 1;
            search1 = search0 + (search1 - search0) / 2;
        } else {
            float temp = search1;
            search1 = search1 + (search1 - search0) / 2;
            search0 = temp;
        }
    }

    float visibility = hit1 * positionTo.w;
    visibility *= (1 - max(dot(-unitPositionFrom, pivot), 0));
    visibility *= (1 - clamp(depth / thickness, 0, 1));\
    visibility *= (1 - clamp(length(positionTo - positionFrom) / maxDistance, 0, 1));
    visibility *= (uv.x < 0 || uv.x > 1 ? 0 : 1) * (uv.y < 0 || uv.y > 1 ? 0 : 1);

    visibility = clamp(visibility, 0, 1);
    uv.ba = vec2(visibility);

    if (uv.b <= 0.0) {
        FragColor = vec4(0.0);
        return;
    }

    vec3 uvColor = texture(colorTexture, uv.xy).rgb;
    float alpha = clamp(uv.b, 0.0, 1.0);

    FragColor = vec4(mix(vec3(0.0), uvColor, 1.0), 1.0);
}