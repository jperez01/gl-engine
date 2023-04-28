#include "gl_funcs.h"
#include "stb_image.h"

#include <glad/glad.h>
#include <iostream>
#include <utility>

namespace glutil {
    AllocatedBuffer createUnitCube() {
        float skyboxVertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
             1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
             1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };
        std::vector<VertexType> parameters = { POSITION, NORMAL, TEXCOORDS};
        std::vector<float> newSkyboxVertices(std::begin(skyboxVertices), std::end(skyboxVertices));
        return glutil::loadVertexBuffer(newSkyboxVertices, parameters);
    }

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

    AllocatedBuffer createPlane() {
        float planeVertices[] = {
            // positions            // normals         // texcoords
            25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  1.0f,  0.0f,
            -25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
            -25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

            25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  1.0f,  0.0f,
            -25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
            25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f
        };
        std::vector<float> newPlaneVertices(std::begin(planeVertices), std::end(planeVertices));
        std::vector<VertexType> parameters = { POSITION, NORMAL, TEXCOORDS};

        return glutil::loadVertexBuffer(newPlaneVertices, parameters);
    }

    unsigned int loadFloatTexture(std::string path, GLenum format, GLenum storageFormat) {
        int width, height, nrComponents;

        float *data = stbi_loadf(path.c_str(), &width, &height, &nrComponents, 0);
        if (data) {
            unsigned int textureID = createTexture(width, height, GL_FLOAT, format, storageFormat, data);
            return textureID;
        } else {
            std::cout << "HDR texture failed to load at path: " << path << std::endl;
            stbi_image_free(data);

            return 0;
        }
    }

    unsigned int loadTexture(std::string path, GLenum dataType, GLenum format, GLenum storageFormat) {
        int width, height, nrComponents;
        
        unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);
        if (data) {
            unsigned int textureID = createTexture(width, height, dataType, format, storageFormat, data);
            return textureID;
        } else {
            std::cout << "Texture failed to load at path: " << path << std::endl;
            stbi_image_free(data);

            return 0;
        }
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

    Texture loadSomeTexture(std::string path) {
        Texture texture;
        int width, height, nrComponents;

        unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);
        if (data) {
            GLenum dataType = GL_UNSIGNED_BYTE;
            unsigned int textureID = createTexture(width, height, dataType, nrComponents, data);
            
            texture.id = textureID;
            texture.height = height;
            texture.width = width;
            texture.nrComponents = nrComponents;

            return texture;
        }
        else {
            std::cout << "Texture failed to load at path: " << path << std::endl;
            stbi_image_free(data);

            return texture;
        }
    }

    unsigned int createTexture3D(int width, int height, int depth, GLenum storageFormat) {
        unsigned int textureID;

        glCreateTextures(GL_TEXTURE_3D, 1, &textureID);

        glTextureParameteri(textureID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        const std::vector<GLfloat> someBuffer(4 * width * height * depth, 0.0f);

        glTextureStorage3D(textureID, 6, storageFormat, width, height, depth);
        glTextureSubImage3D(textureID, 0, 0, 0, 0, width, height, depth, GL_RGBA, GL_FLOAT, &someBuffer[0]);
        glGenerateTextureMipmap(textureID);
        
        return textureID;
    }

    unsigned int createTextureArray(int size, int width, int height, GLenum dataType, GLenum format, GLenum storageFormat, void* data) {
        unsigned int textureID;
        glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &textureID);

        glTextureStorage3D(textureID, 1, storageFormat, width, height, size);

        for (int i = 0; i < size; i++) {
            glTextureSubImage3D(textureID, 0, 0, 0, i, width, height, 1, format, dataType, data);
        }

        glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        return textureID;
    }

    unsigned int createTexture(int width, int height, GLenum dataType, GLenum format, GLenum storageFormat, void* data, int levels) {
        unsigned int textureID;
        glCreateTextures(GL_TEXTURE_2D, 1, &textureID);

        glTextureStorage2D(textureID, levels, storageFormat, width, height);
        glTextureSubImage2D(textureID, 0, 0, 0, width, height, format, dataType, data);

        glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        if (levels > 1) glGenerateTextureMipmap(textureID);

        return textureID;
    }

    unsigned int createTexture(int width, int height, GLenum dataType, int nrComponents, unsigned char* data, int levels) {
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

        glTextureStorage2D(textureID, levels, storageFormat, width, height);
        glTextureSubImage2D(textureID, 0, 0, 0, width, height, format, dataType, data);

        glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        if (levels > 1) glGenerateTextureMipmap(textureID);

        return textureID;
    }

    unsigned int createCubemap(int width, int height, GLenum dataType, GLenum format, GLenum storageFormat, int nrComponents) {
        unsigned int cubemapID;
        
        glGenTextures(1, &cubemapID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapID);

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

            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
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

