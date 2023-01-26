#pragma once

#include <string>
#include <vector>

#include "utils/gl_types.h"

static std::vector<std::string> defaultFaces = {
    "right.jpg",
    "left.jpg",
    "top.jpg",
    "bottom.jpg",
    "back.jpg",
    "front.jpg"
};

namespace glutil {
    unsigned int loadCubemap(std::string path, std::vector<std::string> faces = defaultFaces);

    AllocatedBuffer loadVertexBuffer(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);
    AllocatedBuffer loadSimpleVertexBuffer(std::vector<float>& vertices);
};