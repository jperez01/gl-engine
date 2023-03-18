#pragma once
#include "gl_base_engine.h"

struct DeferredLight {
    glm::vec3 position;
    glm::vec3 color;
};

class DeferredEngine : public GLEngine {
public:
    void init_resources();
    void run();

    void createValues();
    void handleEvents();

private:
    bool shouldAA = false;
    float stepMultiplier = 1.0f;
    std::vector<DeferredLight> lights;

    unsigned int deferredFBO;
    unsigned int gPosition, gNormal, gAlbedo, gReflectionPosition;
    unsigned int depthMap;

    unsigned int screenFBO;
    unsigned int colorTexture;
    glm::vec2 inverseScreenSize;

    unsigned int ssrFBO;
    unsigned int gReflectionColor;

    std::vector<glm::vec3> objectPositions;

    AllocatedBuffer quadBuffer;
    AllocatedBuffer planeBuffer;
    unsigned int planeTexture;

    Shader renderPipeline;
    Shader gbufferPipeline;
    Shader fxaaPipeline;
};