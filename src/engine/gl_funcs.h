#pragma once

#include <string>
#include <vector>

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

namespace glutil {
    AllocatedBuffer createScreenQuad();

    enum VertexType { BASIC = 0, WITH_NORMAL, WITH_TEXCOORDS, WITH_TANGENT, WITH_BTANGENT };

    unsigned int loadTexture(std::string path);
    unsigned int createTexture(int width, int height, GLenum dataType, int nrComponents = 0, unsigned char* data = nullptr);
    unsigned int createTexture(int width, int height, GLenum dataType, GLenum format = GL_RGBA, GLenum storageFormat = GL_RGBA8, unsigned char* data = nullptr);

    unsigned int createCubemap(int width, int height, GLenum dataType, int nrComponents = 0);
    unsigned int loadCubemap(std::string path, std::vector<std::string> faces = defaultFaces);

    AllocatedBuffer loadVertexBuffer(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);
    AllocatedBuffer loadSimpleVertexBuffer(std::vector<float>& vertices, VertexType endpoint = BASIC);

    AllocatedBuffer loadOldVertexBuffer(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);
    AllocatedBuffer loadOldSimpleVertexBuffer(std::vector<float>& vertices, VertexType endpoint = BASIC);
};