#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <unordered_map>

struct SimpleDirectionalLight {
    glm::vec3 direction;

    glm::vec3 color;
};

struct DirLight {
    glm::vec3 direction;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct PointLight {
    glm::vec3 position;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct Sphere {
    glm::vec3 origin;
    float radius;

    glm::vec3 albedo;
    glm::vec3 specular;
};

struct Plane {
    glm::vec3 normal;
    glm::vec3 point;

    glm::vec3 albedo;
    glm::vec3 specular;
};

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
    unsigned int ID;
};

struct Texture {
    unsigned int id = -1;
    std::string type;
    std::string path;

    int width, height, nrComponents;

    unsigned char* data = nullptr;
};

#define MAX_BONES_PER_VERTEX 4

struct VertexBoneData {
    unsigned int boneIDs[MAX_BONES_PER_VERTEX] = {0};
    float weights[MAX_BONES_PER_VERTEX] = {0.0f};
};

void addBoneData(VertexBoneData& data, unsigned int boneID, float weight);

struct BoneInfo {
    glm::mat4 offsetTransform;
    glm::mat4 finalTransform;
};

struct AllocatedBuffer {
    unsigned int VAO, VBO, EBO;
};

struct BoundingBox {
    glm::vec4 minPoint;
    glm::vec4 maxPoint;

    bool isInitialized = false;
};