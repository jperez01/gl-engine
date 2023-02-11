#version 430 core

layout (local_size_x = 8, local_size_y = 4, local_size_z = 1) in;

layout(rgba32f, binding = 0) uniform image2D imgOutput;

uniform float t;
uniform mat4 inverseView;

struct Ray {
    vec3 direction;
    vec3 origin;
};

struct Sphere {
    vec3 origin;
    float radius;

    vec3 albedo;
    vec3 specular;
};

struct RayHit {
    vec3 position;
    float t;
    float energy;
    vec3 normal;

    vec3 albedo;
    vec3 specular;
};

struct PointLight {
    vec3 position;

    vec3 ambient;
    vec3 specular;
    vec3 diffuse;
};

struct DirectionalLight {
    vec3 direction;

    vec3 ambient;
    vec3 specular;
    vec3 diffuse;
};

struct Plane {
    vec3 normal;
    vec3 point;

    vec3 albedo;
    vec3 specular;
};

uniform Sphere spheres[25];
uniform Plane plane;

uniform PointLight pointLights[4];
uniform DirectionalLight dirLight;

uniform int numReflections;
uniform int shininess;

uniform samplerCube cubemap;

void intersectGroundPlane(Ray ray, inout RayHit bestHit) {
    vec3 convertedNormal = normalize(plane.normal);
    float denom = dot(convertedNormal, ray.direction);

    if (abs(denom) > 0.00001) {
        vec3 pointDiff = plane.point - ray.origin;
        float t = dot(pointDiff, convertedNormal) / denom;
        if (t >= 0.0f && t < bestHit.t) {
            bestHit.t = t;
            bestHit.position = ray.origin + t * ray.direction;
            bestHit.normal = convertedNormal;
            bestHit.albedo = plane.albedo;
            bestHit.specular = plane.specular;
        }
    }
}

void intersectSphere(Ray ray, Sphere sphere, inout RayHit bestHit) {
    vec3 from_sphere_to_ray = ray.origin - sphere.origin;

    float a = dot(ray.direction, ray.direction);
    float b = 2 * dot(ray.direction, from_sphere_to_ray);
    float c = dot(from_sphere_to_ray, from_sphere_to_ray) - sphere.radius;

    float discriminant = b * b - 4 * a * c;
    float sqrt_disc = sqrt(discriminant);
    if (discriminant < 0) return;

    float t1 = (-b + sqrt_disc) / (2 * a);
    float t2 = (-b - sqrt_disc) / (2 * a);
    float t = (t1 > t2) ? t2 : t1;

    if (t > 0.0f && t < bestHit.t) {
        bestHit.t = t;
        bestHit.position = ray.origin + t * ray.direction;
        bestHit.normal = normalize(bestHit.position - sphere.origin);
        bestHit.albedo = sphere.albedo;
        bestHit.specular = sphere.specular;
    }
}

vec3 shadeDirLight(RayHit hit, DirectionalLight light) {
    vec3 lightDir = normalize(-light.direction);

    Ray someRay = Ray(lightDir, hit.position + hit.normal * 0.001f);
    RayHit tempHit;
    tempHit.t = 10000.0f;

    intersectGroundPlane(someRay, tempHit);
    if (tempHit.t > 0.001f && tempHit.t < 10000.0f) return vec3(0.0f);

    for (int j = 0; j < 25; j++) {
        intersectSphere(someRay, spheres[j], tempHit);
        if (tempHit.t > 0.001f && tempHit.t < 10000.0f) return vec3(0.0f);
    }

    float diff = max(dot(hit.normal, lightDir), 0.0);

    vec3 reflectDir = reflect(-lightDir, hit.normal);
    float spec = pow(max(dot(hit.normal, reflectDir), 0.0), shininess);

    vec3 diffuse = diff * light.diffuse * hit.albedo;
    vec3 specular = spec * light.specular * hit.specular;
    vec3 ambient = light.ambient * hit.albedo;

    return (diffuse + specular + ambient);
}

vec3 shadePointLight(RayHit hit, PointLight light) {
    vec3 lightDir = normalize(light.position - hit.position);

    Ray someRay = Ray(lightDir, hit.position + hit.normal * 0.001f);
    RayHit tempHit;
    tempHit.t = 10000.0f;

    intersectGroundPlane(someRay, tempHit);
    if (tempHit.t > 0.001f && tempHit.t < 10000.0f) return vec3(0.0f);

    for (int j = 0; j < 25; j++) {
        intersectSphere(someRay, spheres[j], tempHit);
        if (tempHit.t > 0.001f && tempHit.t < 10000.0f) return vec3(0.0f);
    }

    float diff = max(dot(hit.normal, lightDir), 0.0);

    vec3 reflectDir = reflect(-lightDir, hit.normal);
    float spec = pow(max(dot(hit.normal, reflectDir), 0.0), shininess);

    vec3 diffuse = diff * light.diffuse * hit.albedo;
    vec3 specular = spec * light.specular * hit.specular;
    vec3 ambient = light.ambient * hit.albedo;

    return (diffuse + specular + ambient);
}

vec3 shade(RayHit hit) {
    vec3 totalColor = vec3(0.0f);
    for (int i = 0; i < 4; i++) {
        totalColor += shadePointLight(hit, pointLights[i]);
    }
    totalColor += shadeDirLight(hit, dirLight);

    return totalColor;
}

float convertRange(float value, float offset, float scale) {
    return (value - offset) * scale;
}

void main() {
    vec4 value = vec4(0.0, 0.0, 0.0, 1.0);
    ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);
    float speed = 100, width = 2048;

    value.x = mod(float(texelCoord.x) + t * speed, width) / (gl_NumWorkGroups.x * gl_WorkGroupSize.x);
    value.y = float(texelCoord.y) / (gl_NumWorkGroups.y * gl_WorkGroupSize.y);
    value.z = mod(t / width, width);

    float x = (float(texelCoord.x * 2 - width) / width);
    float y = (float(texelCoord.y * 2 - width) / width);

    vec3 pixel = vec3(inverseView * vec4(x * 2.0, y * 2.0, 0.0, 1.0));
    vec3 origin = vec3(inverseView * vec4(0, 0, -1.0, 1.0));
    vec3 direction = normalize(pixel - origin);
    Ray newRay = Ray(direction, origin);

    RayHit hit;
    hit.t = 10000.0f;
    hit.energy = 1.0f;
    vec3 totalColor = vec3(0.0f);

    for (int i = 0; i < numReflections; i++) {
        intersectGroundPlane(newRay, hit);
        for (int j = 0; j < 25; j++) {
            intersectSphere(newRay, spheres[j], hit);
        }
        if (hit.t < 0.001 || hit.t > 500.0f)  {
            totalColor += hit.energy * texture(cubemap, newRay.direction).xyz;
            break;
        } else {
            totalColor += hit.energy * shade(hit);

            newRay.origin = hit.position + hit.normal * 0.0001f;
            newRay.direction = reflect(newRay.direction, hit.normal);

            hit.t = 10000.0f;
            hit.energy *= 0.5;
        }
    }

    imageStore(imgOutput, texelCoord, vec4(totalColor, 1.0));
}