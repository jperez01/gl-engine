#include "gl_funcs.h"
#include "stb_image.h"

#include <glad/glad.h>
#include <iostream>

namespace glutil {
    unsigned int loadCubemap(std::string path, std::vector<std::string> faces) {
        unsigned int textureID;
        
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &textureID);
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

                /** 
                 * if (i == 0) glTextureStorage2D(textureID, 1, storageFormat, width, height);
                glTextureSubImage3D(textureID, 0, 0, 0, i, width, height, 1, format, GL_UNSIGNED_BYTE, data);
                glGenerateTextureMipmap(textureID);

                glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTextureParameteri(textureID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
                glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                **/
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

    AllocatedBuffer loadSimpleVertexBuffer(std::vector<float>& vertices) {
        GLuint binding_index = 0;
        unsigned int VAO, VBO;

        glCreateBuffers(1, &VBO);
        glNamedBufferStorage(VBO, sizeof(float) * vertices.size(), vertices.data(), GL_DYNAMIC_STORAGE_BIT);

        glCreateVertexArrays(1, &VAO);

        glEnableVertexArrayAttrib(VAO, 0);
        glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(VAO, 0, binding_index);

        glVertexArrayVertexBuffer(VAO, binding_index, VBO, 0, sizeof(float) * 3);

        AllocatedBuffer newBuffer;
        newBuffer.VAO = VAO;
        newBuffer.VBO = VBO;

        return newBuffer;
    }
};

