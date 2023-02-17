#pragma once

#include <string>
#include <vector>
#include <map>

#include "utils/gl_types.h"
#include <glad/glad.h>

static std::vector<std::string> defaultFaces = {
    "right.jpg",
    "left.jpg",
    "top.jpg",
    "bottom.jpg",
    "front.jpg",
    "back.jpg"
};

enum VertexType { POSITION = 0, NORMAL, TEXCOORDS, TANGENT, BI_TANGENT, VERTEX_ID };

static std::map<VertexType, int> sizes = {
    {POSITION, 3},
    {NORMAL, 3},
    {TEXCOORDS, 2},
    {TANGENT, 3},
    {BI_TANGENT, 3},
    {VERTEX_ID, 1}
};

static std::vector<VertexType> basicEndpoints = {
    POSITION, NORMAL, TEXCOORDS
};

namespace glutil {
    AllocatedBuffer createScreenQuad();

    unsigned int loadTexture(std::string path);
    unsigned int createTexture(int width, int height, GLenum dataType, int nrComponents = 0, unsigned char* data = nullptr);
    unsigned int createTexture(int width, int height, GLenum dataType, GLenum format = GL_RGBA, GLenum storageFormat = GL_RGBA8, unsigned char* data = nullptr);

    unsigned int createCubemap(int width, int height, GLenum dataType, int nrComponents = 0);
    unsigned int loadCubemap(std::string path, std::vector<std::string> faces = defaultFaces);

    AllocatedBuffer loadVertexBuffer(std::vector<float>& vertices, std::vector<VertexType>& endpoints = basicEndpoints);
    AllocatedBuffer loadVertexBuffer(std::vector<float>& vertices, std::vector<unsigned int>& indices, std::vector<VertexType>& endpoints = basicEndpoints);
    AllocatedBuffer loadVertexBuffer(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, std::vector<VertexType>& endpoints = basicEndpoints);
};