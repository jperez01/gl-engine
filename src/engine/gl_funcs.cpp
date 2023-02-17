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
        quadBuffer = glutil::loadVertexBuffer(newQuadVertices);

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
    /**
     * AllocatedBuffer loadSimpleVertexBuffer(std::vector<float>& vertices, VertexType endpoint) {
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
     */
    AllocatedBuffer loadVertexBuffer(std::vector<float>& vertices, std::vector<VertexType>& endpoints) {
        unsigned int VAO, VBO;

        glCreateVertexArrays(1, &VAO);

        glCreateBuffers(1, &VBO);
        glNamedBufferStorage(VBO, sizeof(float) * vertices.size(), vertices.data(), GL_DYNAMIC_STORAGE_BIT);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        int totalLength = 0;
        for (int i = 0; i < endpoints.size(); i++) {
            totalLength += sizes[endpoints[i]];
        }

        int currentOffset = 0;
        for (int i = 0; i < endpoints.size(); i++) {
            VertexType type = endpoints[i];

            glEnableVertexAttribArray(i);
            glVertexAttribPointer(i, sizes[type], GL_FLOAT, GL_FALSE, sizeof(float) * totalLength, (void*) (currentOffset * sizeof(float)));

            currentOffset += sizes[type];
        }

        AllocatedBuffer newBuffer;
        newBuffer.VAO = VAO;
        newBuffer.VBO = VBO;

        return newBuffer;
    }

    AllocatedBuffer loadVertexBuffer(std::vector<float>& vertices, std::vector<unsigned int>& indices, 
        std::vector<VertexType>& endpoints) {

        GLuint binding_index = 0;
        unsigned int VAO, VBO, EBO;

        glCreateVertexArrays(1, &VAO);

        glCreateBuffers(1, &VBO);
        glNamedBufferStorage(VBO, sizeof(float) * vertices.size(), vertices.data(), GL_DYNAMIC_STORAGE_BIT);

        glCreateBuffers(1, &EBO);
        glNamedBufferStorage(EBO, sizeof(unsigned int) * indices.size(), indices.data(), GL_DYNAMIC_STORAGE_BIT);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        int totalLength = 0;
        for (int i = 0; i < endpoints.size(); i++) {
            totalLength += sizes[endpoints[i]];
        }

        int currentOffset = 0;
        for (int i = 0; i < endpoints.size(); i++) {
            VertexType type = endpoints[i];

            glEnableVertexAttribArray(i);
            glVertexAttribPointer(i, sizes[type], GL_FLOAT, GL_FALSE, sizeof(float) * totalLength, (void*) (currentOffset * sizeof(float)));

            currentOffset += sizes[type];
        }

        glVertexArrayElementBuffer(VAO, EBO);

        AllocatedBuffer newBuffer;
        newBuffer.VAO = VAO;
        newBuffer.VBO = VBO;
        newBuffer.EBO = EBO;

        return newBuffer;
    }

    AllocatedBuffer loadVertexBuffer(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, 
        std::vector<VertexType>& endpoints) {

        GLuint binding_index = 0;
        unsigned int VAO, VBO, EBO;

        glCreateVertexArrays(1, &VAO);

        glCreateBuffers(1, &VBO);
        glNamedBufferStorage(VBO, sizeof(Vertex) * vertices.size(), vertices.data(), GL_DYNAMIC_STORAGE_BIT);

        glCreateBuffers(1, &EBO);
        glNamedBufferStorage(EBO, sizeof(unsigned int) * indices.size(), indices.data(), GL_DYNAMIC_STORAGE_BIT);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        int totalLength = 0;
        for (int i = 0; i < endpoints.size(); i++) {
            totalLength += sizes[endpoints[i]];
        }

        int currentOffset = 0;
        for (int i = 0; i < endpoints.size(); i++) {
            VertexType type = endpoints[i];

            glEnableVertexAttribArray(i);
            if (i == 5) {
                glVertexAttribIPointer(i, sizes[type], GL_UNSIGNED_INT, sizeof(unsigned int) * totalLength, (void*) (currentOffset * sizeof(float)));
            } else {
                glVertexAttribPointer(i, sizes[type], GL_FLOAT, GL_FALSE, sizeof(float) * totalLength, (void*) (currentOffset * sizeof(float)));
            }


            currentOffset += sizes[type];
        }

        glVertexArrayElementBuffer(VAO, EBO);

        AllocatedBuffer newBuffer;
        newBuffer.VAO = VAO;
        newBuffer.VBO = VBO;
        newBuffer.EBO = EBO;

        return newBuffer;
    }
};

