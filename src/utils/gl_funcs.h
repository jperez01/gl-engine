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
    AllocatedBuffer createUnitCube();
    AllocatedBuffer createScreenQuad();
    AllocatedBuffer createPlane();

    unsigned int loadFloatTexture(std::string path, GLenum format, GLenum storageFormat);
    unsigned int loadTexture(std::string path, GLenum dataType, GLenum format, GLenum storageFormat);
    unsigned int loadTexture(std::string path);
    Texture loadSomeTexture(std::string path);

    unsigned int createTexture3D(int width, int height, int depth, GLenum storageFormat = GL_RGBA8);
    unsigned int createTextureArray(int size, int width, int height, GLenum dataType, GLenum format = GL_RGBA, GLenum storageFormat = GL_RGBA8, void* data = nullptr);
    unsigned int createTexture(int width, int height, GLenum dataType, int nrComponents = 0, unsigned char* data = nullptr, int levels = 4);
    unsigned int createTexture(int width, int height, GLenum dataType, GLenum format = GL_RGBA, GLenum storageFormat = GL_RGBA8, void* data = nullptr, int levels = 4);

    unsigned int createCubemap(int width, int height, GLenum dataType, GLenum format = GL_DEPTH_COMPONENT, GLenum storageFormat = GL_DEPTH_COMPONENT, int nrComponents = -1);
    unsigned int loadCubemap(std::string path, std::vector<std::string> faces = defaultFaces);

    AllocatedBuffer loadVertexBuffer(std::vector<float>& vertices, std::vector<VertexType>& endpoints = basicEndpoints);
    AllocatedBuffer loadVertexBuffer(std::vector<float>& vertices, std::vector<unsigned int>& indices, std::vector<VertexType>& endpoints = basicEndpoints);
    AllocatedBuffer loadVertexBuffer(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, std::vector<VertexType>& endpoints = basicEndpoints);
};