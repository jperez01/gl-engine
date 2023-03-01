#version 460 core

layout (location = 0) out vec4 FragColor;

noperspective in vec2 TexCoords;

uniform sampler2D depthTexture;
uniform sampler2D positionTexture;
uniform sampler2D normalTexture;
uniform sampler2D colorTexture;

uniform mat4 projection;
uniform mat4 invProjection;
uniform mat4 view;
uniform mat4 invView;
uniform mat4 invTransposeView;

uniform float depthCutoff = 0.0;

const float rayStep = 0.1;
const float minRayStep = 0.1;
uniform float maxRayStep = 1.2;
const int maxSteps = 30;
const float searchDist = 5;
const int numBinarySearchSteps = 5;
const float reflectionSpecularFalloffExponent = 3.0;

#define Scale vec3(.8, .8, .8)
#define K 19.19

vec4 rayCast(in vec3 dir, inout vec3 hitCoord, out float dDepth);
vec3 binarySearch(inout vec3 dir, inout vec3 hitCoord, inout float dDepth);
vec3 hash(vec3 a);

vec3 positionFromDepth(float depth) {
	float z = depth * 2.0 - 1.0;

	vec4 clipSpacePosition = vec4(TexCoords * 2.0 - 1.0, z, 1.0);
	vec4 viewSpacePosition = invProjection * clipSpacePosition;
	viewSpacePosition /= viewSpacePosition.w;

	return viewSpacePosition.xyz;
}

bool isSignificant(float dd) {
	return dd < maxRayStep && dd > depthCutoff;
}

void main() {
	float currentDepth = texture(depthTexture, TexCoords).x;
	vec3 depthPos = positionFromDepth(currentDepth);

	if (currentDepth > 0.999999) {
		FragColor = vec4(0.0);
		return;
	}

	vec3 viewNormal = vec3(texture(normalTexture, TexCoords));
	vec3 viewPos = texture(positionTexture, TexCoords).xyz;
	vec3 albedo = texture(colorTexture, TexCoords).xyz;
	float spec = texture(colorTexture, TexCoords).w;

	vec3 reflected = normalize(reflect(normalize(viewPos), normalize(viewNormal)));
	vec3 hitPos = viewPos;
	float dDepth;

	vec3 worldPos = vec3(vec4(viewPos, 1.0) * invView);
	vec3 jitt = mix(vec3(0.0), vec3(hash(worldPos)), spec);

	vec4 coords = rayCast(normalize(jitt + reflected) * max(minRayStep, -viewPos.z), hitPos, dDepth);

	vec2 dCoords = smoothstep(0.2, 0.6, abs(vec2(0.5) - coords.xy));

	float screenEdgeFactor = clamp(1.0 - (dCoords.x + dCoords.y), 0.0, 1.0);

	float multiplier = screenEdgeFactor * -reflected.z;

	vec3 SSR = texture(colorTexture, coords.xy).rgb * clamp(multiplier, 0.0, 0.9);

	FragColor = vec4(SSR, 1.0);
}

vec4 rayCast(in vec3 dir, inout vec3 hitCoord, out float dDepth) {
	dir *= rayStep;

	float depth = 0.0;
	int steps = 0;
	vec4 projectedCoord = vec4(0.0);

	for (int i = 0; i < maxSteps; i++) {
		hitCoord += dir;
		
		projectedCoord = projection * vec4(hitCoord, 1.0);
		projectedCoord.xy /= projectedCoord.w;
		projectedCoord.xy = projectedCoord.xy * 0.5 + 0.5;

		depth = texture(positionTexture, projectedCoord.xy).z;

		if (depth > 1000.0) continue;

		dDepth = hitCoord.z - depth;
		if (dDepth <= 0.0 && (dir.z - dDepth) < maxRayStep) {
			vec4 result = vec4(binarySearch(dir, hitCoord, dDepth), 1.0);
			return result;
		}
		steps++;
	}
	if (depth < 1000.0) return vec4(0.0);

	return vec4(projectedCoord.xy, depth, 0.0);
}

vec3 binarySearch(inout vec3 dir, inout vec3 hitCoord, inout float dDepth) {
	float depth;
	vec4 projectedCoord;

	for (int i = 0; i < numBinarySearchSteps; i++) {
		projectedCoord = projection * vec4(hitCoord, 1.0);
		projectedCoord.xy /= projectedCoord.w;
		projectedCoord.xy = projectedCoord.xy * 0.5 + 0.5;

		depth = texture(positionTexture, projectedCoord.xy).z;

		dDepth = hitCoord.z - depth;

		dir *= 0.5;

		hitCoord += (dDepth > 0.0) ? dir : -dir;
	}

	projectedCoord = projection * vec4(hitCoord, 1.0);
	projectedCoord.xy /= projectedCoord.w;
	projectedCoord.xy = projectedCoord.xy * 0.5 + 0.5;

	return vec3(projectedCoord.xy, dDepth);
}

vec3 hash(vec3 a)
{
    a = fract(a * Scale);
    a += dot(a, a.yxz + K);
    return fract((a.xxy + a.yxx)*a.zyx);
}