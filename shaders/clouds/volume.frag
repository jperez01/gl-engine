#version 430 core

in vec3 WorldPos;
out vec4 FragColor;

uniform sampler3D noiseTexture;

uniform mat4 inverseModel;

uniform vec3 cameraPos;
uniform vec3 boundsMin;
uniform vec3 boundsMax;
uniform int numSteps;

uniform vec3 cloudColor;
uniform float cloudScale;
uniform float cloudOffset;
uniform float densityThreshold;
uniform float densityMultiplier;

uniform vec3 backgroundColor;
uniform vec3 lightPos;
uniform int numStepsLight;
uniform float lightAbsorptionTowardsSun;
uniform float darknessThreshold;
uniform float lightAbsorptionThroughCloud;
uniform vec4 phaseParams;

float hg(float a, float g) {
    float g2 = g*g;
    return (1-g2) / (4*3.1415*pow(1+g2-2*g*(a), 1.5));
}

float phase(float a) {
    float blend = .5;
    float hgBlend = hg(a,phaseParams.x) * (1-blend) + hg(a,-phaseParams.y) * blend;
    return phaseParams.z + hgBlend*phaseParams.w;
}

vec2 rayBoxDistance(vec3 boxMin, vec3 boxMax, vec3 rayOrigin, vec3 rayDirection) {
	vec3 t0 = (boxMin - rayOrigin) / rayDirection;
	vec3 t1 = (boxMax - rayOrigin) / rayDirection;

	vec3 tmin = min(t0, t1);
	vec3 tmax = max(t0, t1);

	float distA = max(tmin.x, max(tmin.y, tmin.z));
	float distB = min(tmax.x, min(tmax.y, tmax.z));
	/*
	Case 1: if ray intersects box from outside (0 <= distA <= distB)
	distA is dst to nearest intersection, distB is dst to farthest intersection

	Case 2: if ray intersects box from inside (distA < 0 < distB)
	distA is dst to intersection behind the ray, distB is dst to intersection ahead of ray

	case 3: if ray does not intersection box (distA > distB)
	*/

	float distToBox = max(0.0, distA);
	float distInsideBox = max(0.0, distB - distToBox);

	return vec2(distToBox, distInsideBox);
}

float sampleDensity(vec3 position) {
	vec3 convertedPos = vec3(inverseModel * vec4(position, 1.0));
	vec3 uvw = convertedPos * cloudScale * 0.1 + cloudOffset * 0.01;
	float shape = texture(noiseTexture, uvw).r;
	float density = max(0.0, shape.r - densityThreshold) * densityMultiplier;

	return density;
}

float lightmarch(vec3 position) {
	vec3 dirToLight = normalize(lightPos - position);
	float distanceInsideBox = rayBoxDistance(boundsMin, boundsMax, position, dirToLight).y;

	float stepSize = distanceInsideBox / numStepsLight;
	float totalDensity = 0;

	for (int i = 0; i < numStepsLight; i++) {
		position += dirToLight * stepSize;
		totalDensity += max(0, sampleDensity(position) * stepSize);
	}

	float transmittance = exp(-totalDensity * lightAbsorptionTowardsSun);

	return darknessThreshold + transmittance * (1 - darknessThreshold);
}

void main() {
	vec3 rayOrigin = cameraPos;
	vec3 rayDirection = normalize(WorldPos - cameraPos);

	vec2 rayBoxInfo = rayBoxDistance(boundsMin, boundsMax, rayOrigin, rayDirection);
	float distToBox = rayBoxInfo.x, distInsideBox = rayBoxInfo.y;

	float distanceTravelled = 0.0, distanceLimit = distInsideBox;
	float stepSize = distInsideBox / numSteps;

	vec3 lightEnergy = vec3(0.0);
	float transmittence = 1.0;

	vec3 lightDir = WorldPos - lightPos;
	float cosAngle = dot(rayDirection, lightDir);
	float phaseVal = phase(cosAngle);

	float totalDensity = 0.0;
	while (distanceTravelled < distanceLimit) {
		vec3 currentPos = rayOrigin + rayDirection * (distToBox + distanceTravelled);

		float density = sampleDensity(currentPos);

		if (density > 0.0) {
			float lightTransmittance = lightmarch(currentPos);
			lightEnergy += density * stepSize * transmittence * lightTransmittance * phaseVal;
			transmittence *= exp(-density * stepSize * lightAbsorptionThroughCloud);

			if (transmittence < 0.01) break;
		}
		distanceTravelled += stepSize;
	}

	vec3 finalCloudColor = lightEnergy * cloudColor;
	vec3 finalColor = transmittence * backgroundColor + finalCloudColor;

	FragColor = vec4(finalColor, 1.0);
}