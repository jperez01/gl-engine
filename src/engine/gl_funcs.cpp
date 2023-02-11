#include "gl_funcs.h"
#include "stb_image.h"

#include <glad/glad.h>
#include <iostream>
#include <utility>

namespace glutil {
    AllocatedBuffer createScreenQuad() {
        AllocatedBuffer quadBuffer;
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        };
        std::vector<float> newQuadVertices(std::begin(quadVertices), std::end(quadVertices));
        quadBuffer = glutil::loadOldSimpleVertexBuffer(newQuadVertices, glutil::WITH_TEXCOORDS);

        return quadBuffer;
    }

    unsigned int loadTexture(std::string path) {
        int width, height, nrComponents;

        unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);
        if (data) {
            GLenum dataType = GL_UNSIGNED_BYTE;
            unsigned int textureID = createTexture(width, height, dataType, nrComponents, data);
            return textureID;
        } else {
            std::cout << "Texture failed to load at path: " << path << std::endl;
            stbi_image_free(data);

            return 0;
        }
    }

    unsigned int createTexture(int width, int height, GLenum dataType, GLenum format, GLenum storageFormat, unsigned char* data) {
        unsigned int textureID;
        glCreateTextures(GL_TEXTURE_2D, 1, &textureID);

        glTextureStorage2D(textureID, 1, storageFormat, width, height);
        glTextureSubImage2D(textureID, 0, 0, 0, width, height, format, dataType, data);

        glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        return textureID;
    }

    unsigned int createTexture(int width, int height, GLenum dataType, int nrComponents, unsigned char* data) {
        unsigned int textureID;
        glCreateTextures(GL_TEXTURE_2D, 1, &textureID);

        GLenum format;
        GLenum storageFormat;
        if (nrComponents == 0) {
            format = GL_DEPTH_COMPONENT;
            storageFormat = GL_DEPTH_COMPONENT;
        } else if (nrComponents == 1) {
            format = GL_RED;
            storageFormat = GL_R8;
        } else if (nrComponents == 3) {
            format = GL_RGB;
            storageFormat = GL_RGB8;
        } else if (nrComponents == 4) {
            format = GL_RGBA;
            storageFormat = GL_RGBA8;
        }

        glTextureStorage2D(textureID, 1, storageFormat, width, height);
        glTextureSubImage2D(textureID, 0, 0, 0, width, height, format, dataType, data);

        glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        return textureID;
    }

    unsigned int createCubemap(int width, int height, GLenum dataType, int nrComponents) {
        unsigned int cubemapID;
        
        glGenTextures(1, &cubemapID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapID);

        GLenum format;
        GLenum storageFormat;
        if (nrComponents == 0) {
            format = GL_DEPTH_COMPONENT;
            storageFormat = GL_DEPTH_COMPONENT;
        } else if (nrComponents == 1) {
            format = GL_RED;
            storageFormat = GL_R8;
        } else if (nrComponents == 3) {
            format = GL_RGB;
            storageFormat = GL_RGB8;
        } else if (nrComponents == 4) {
            format = GL_RGBA;
            storageFormat = GL_RGBA8;
        }

        for (int i = 0; i < 6; i++) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, storageFormat, width, height, 0,
                format, dataType, nullptr);

            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE); 
        }

        return cubemapID;
    }

    unsigned int loadCubemap(std::string path, std::vector<std::string> faces) {
        unsigned int textureID;
        
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

        int width, height, nrChannels;

        GLenum format;
        GLenum storageFormat;

        for (int i = 0; i < 6; i++) {
            std::string facePath = path + faces[i];
            unsigned char* data = stbi_load(facePath.c_str(), &width, &height, &nrChannels, 0);

            if (data) {
                if (nrChannels == 1) {
                    format = GL_RED;
                    storageFormat = GL_R8;
                } else if (nrChannels == 3) {
                    format = GL_RGB;
                    storageFormat = GL_RGB8;
                } else if (nrChannels == 4) {
                    format = GL_RGBA;
                    storageFormat = GL_RGBA8;
                }

               glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, storageFormat, width, height, 0, format,
                GL_UNSIGNED_BYTE, data);

                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

                stbi_image_free(data);
            } else {
                std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
                stbi_image_free(data);
            }
        }

        return textureID;
    }

    AllocatedBuffer loadVertexBuffer(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
        GLuint binding_index = 0;
        unsigned int VAO, EBO, VBO;

        glCreateBuffers(1, &VBO);
        glNamedBufferStorage(VBO, sizeof(Vertex) * vertices.size(), vertices.data(), GL_DYNAMIC_STORAGE_BIT);

        glCreateBuffers(1, &EBO);
        glNamedBufferStorage(EBO, sizeof(unsigned int) * indices.size(), indices.data(), GL_DYNAMIC_STORAGE_BIT);

        glCreateVertexArrays(1, &VAO);

        glEnableVertexArrayAttrib(VAO, 0);
        glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(VAO, 0, binding_index);

        glEnableVertexArrayAttrib(VAO, 1);
        glVertexArrayAttribFormat(VAO, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, Normal));
        glVertexArrayAttribBinding(VAO, 1, binding_index);

        glEnableVertexArrayAttrib(VAO, 2);
        glVertexArrayAttribFormat(VAO, 2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, TexCoords));
        glVertexArrayAttribBinding(VAO, 2, binding_index);

        glEnableVertexArrayAttrib(VAO, 3);
        glVertexArrayAttribFormat(VAO, 3, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, Tangent));
        glVertexArrayAttribBinding(VAO, 3, binding_index);

        glEnableVertexArrayAttrib(VAO, 4);
        glVertexArrayAttribFormat(VAO, 4, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, Bitangent));
        glVertexArrayAttribBinding(VAO, 4, binding_index);

        glVertexArrayVertexBuffer(VAO, binding_index, VBO, 0, sizeof(Vertex));
        glVertexArrayElementBuffer(VAO, EBO);

        AllocatedBuffer newBuffer;
        newBuffer.VAO = VAO;
        newBuffer.EBO = EBO;
        newBuffer.VBO = VBO;

        return newBuffer;
    }

    AllocatedBuffer loadSimpleVertexBuffer(std::vector<float>& vertices, VertexType endpoint) {
        int lengths[5] = { 3, 3, 2, 3, 3};

        GLuint binding_index = 0;
        unsigned int VAO, VBO;

        glCreateVertexArrays(1, &VAO);

        glCreateBuffers(1, &VBO);
        glNamedBufferStorage(VBO, sizeof(float) * vertices.size(), vertices.data(), GL_DYNAMIC_STORAGE_BIT);

        int totalLength = 0;
        for (int i = 0; i <= endpoint; i++) {
            glEnableVertexArrayAttrib(VAO, i);
            glVertexArrayAttribFormat(VAO, i, lengths[i], GL_FLOAT, GL_FALSE, 0);
            glVertexArrayAttribBinding(VAO, i, binding_index);

            totalLength += lengths[i];
        }

        glVertexArrayVertexBuffer(VAO, binding_index, VBO, 0, sizeof(float) * totalLength);

        AllocatedBuffer newBuffer;
        newBuffer.VAO = VAO;
        newBuffer.VBO = VBO;

        return newBuffer;
    }

    AllocatedBuffer loadOldVertexBuffer(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
        GLuint binding_index = 0;
        unsigned int VAO, EBO, VBO;

        glCreateBuffers(1, &VBO);
        glNamedBufferStorage(VBO, sizeof(Vertex) * vertices.size(), vertices.data(), GL_DYNAMIC_STORAGE_BIT);

        glCreateBuffers(1, &EBO);
        glNamedBufferStorage(EBO, sizeof(unsigned int) * indices.size(), indices.data(), GL_DYNAMIC_STORAGE_BIT);

        glCreateVertexArrays(1, &VAO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, Normal));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, TexCoords));
        // vertex tangent
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
        // vertex bitangent
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));

        glVertexArrayElementBuffer(VAO, EBO);

        AllocatedBuffer newBuffer;
        newBuffer.VAO = VAO;
        newBuffer.EBO = EBO;
        newBuffer.VBO = VBO;

        return newBuffer;
    }
    AllocatedBuffer loadOldSimpleVertexBuffer(std::vector<float>& vertices, VertexType endpoint) {
        int lengths[5] = { 3, 3, 2, 3, 3};

        GLuint binding_index = 0;
        unsigned int VAO, VBO;

        glCreateVertexArrays(1, &VAO);

        glCreateBuffers(1, &VBO);
        glNamedBufferStorage(VBO, sizeof(float) * vertices.size(), vertices.data(), GL_DYNAMIC_STORAGE_BIT);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        int totalLength = 0;
        for (int i = 0; i <= endpoint; i++) {
            totalLength += lengths[i];
        }

        int currentOffset = 0;
        for (int i = 0; i <= endpoint; i++) {
            glEnableVertexAttribArray(i);
            glVertexAttribPointer(i, lengths[i], GL_FLOAT, GL_FALSE, sizeof(float) * totalLength, (void*) (currentOffset * sizeof(float)));

            currentOffset += lengths[i];
        }

        AllocatedBuffer newBuffer;
        newBuffer.VAO = VAO;
        newBuffer.VBO = VBO;

        return newBuffer;
    }
};

