#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

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
};

struct Texture {
    unsigned int id;
    std::string type;
    std::string path;

    int width, height, nrComponents;

    unsigned char* data = nullptr;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    std::vector<std::string> texture_paths;

    unsigned int VAO, VBO, EBO;
};

struct AllocatedBuffer {
    unsigned int VAO, VBO, EBO;
};