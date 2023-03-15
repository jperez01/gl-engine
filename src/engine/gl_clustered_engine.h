#pragma once

#include "gl_base_engine.h"
#include "utils/gl_compute.h"

struct ClusteredLight {
    glm::vec4 position;
    glm::vec4 color;
    unsigned int enabled;
    float intensity;
    float range;
    float something;
};

struct ScreenToView {
    glm::mat4 inverseProj;
    glm::uvec4 tileSizes;
    glm::uvec2 screenDimensions;
    glm::uvec2 tileScreenSizes;
};

class ClusteredEngine : public GLEngine {
public:
    void init_resources();
    void run();

private:
    void init_SSBOs();

    std::vector<ClusteredLight> lights;
    unsigned int numLights = 16 * 9 * 4;
    unsigned int maxLights = 1000;
    unsigned int maxLightsPerTile = 50;
    float bias = 0.01f, scale = 1.0f;

    ComputeShader tileCreateCompute;
    ComputeShader clusterLightCompute;

    const unsigned int gridSizeX = 16, gridSizeY = 9, gridSizeZ = 24;
    const unsigned int numClusters = gridSizeX * gridSizeY * gridSizeZ;

    unsigned int deferredFBO;
    unsigned int gPosition, gNormal, gAlbedo, gReflectionPosition;
    unsigned int depthMap;

    unsigned int AABBGridSSBO, screenToViewSSBO, lightSSBO, 
        lightIndicesSSBO, lightGridSSBO, lightGlobalCountSSBO;

    AllocatedBuffer quadBuffer;
    AllocatedBuffer cubeBuffer;

    Shader renderPipeline;
    Shader gBufferPipeline;
    Shader lightBoxPipeline;
};