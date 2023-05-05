#pragma once

#include "gl_base_engine.h"

struct IndirectCommandData {
    unsigned int indexCount;
    unsigned int instanceCount;
    unsigned int firstIndex;
    int baseVertex;
    unsigned int baseInstance = 0;
};

struct IndirectArraysCommandData {
    unsigned int vertexCount;
    unsigned int instanceCount;
    unsigned int firstVertex;
    unsigned int baseInstance = 0;
};

class IndirectEngine : public GLEngine {
public:
    void init_resources();
    void render(std::vector<Model>& objs);
    void handleImGui();
    void handleObjs(std::vector<Model>& objs);

private:
    GLuint indirectDrawBuffer;
    std::vector<IndirectCommandData> mIndirectCommands;

    GLuint textureHandleBuffer, numTextureBuffer;
    std::vector<GLuint64> mTextureHandles;
    std::vector<unsigned int> mNumTextures;

    DirLight directionalLight;

    int index = 0;


    AllocatedBuffer simpleBuffer;
    Shader simplePipeline;
};