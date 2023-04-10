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
    void handleImGui();

    void createValues();
    float createHaltonSequence(unsigned int index, int base);

    void renderFXAA();
    void renderTAA();

private:
    bool shouldFXAA = false;
    float stepMultiplier = 1.0f;
    std::vector<DeferredLight> lights;
    float globalRadius = 100.0f;
    SimpleDirectionalLight directionalLight;

    unsigned int deferredFBO;
    unsigned int gPosition, gNormal, gAlbedo, gReflectionPosition, gMetallic, gVelocity;
    unsigned int depthMap;

    unsigned int aaFBO;
    unsigned int colorTexture;
    glm::vec2 inverseScreenSize;

    unsigned int ssrFBO;
    unsigned int gReflectionColor;

    AllocatedBuffer quadBuffer;

    AllocatedBuffer planeBuffer;
    unsigned int planeTexture;
    glm::mat4 planeModel;

    Shader renderPipeline;
    Shader gbufferPipeline;
    Shader fxaaPipeline;
    Shader ssrPipeline;

    Shader taaResolvePipeline;
    Shader taaHistoryPipeline;
    glm::mat4 prevProjection = glm::mat4(1.0f);
    glm::mat4 prevView = glm::mat4(1.0f);
    int jitterIndex = 0;
    glm::vec2 haltonSequences[128];
    glm::vec2 jitter;

    float multiplier = 0.01;
    ImGuizmo::OPERATION operation = ImGuizmo::OPERATION::TRANSLATE;

    unsigned int simpleColorFBO;
    unsigned int simpleColorTexture;

    unsigned int historyFBO;
    unsigned int historyColorTexture;
};