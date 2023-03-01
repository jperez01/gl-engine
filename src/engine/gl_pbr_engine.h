#pragma once

#include "gl_base_engine.h"

#include <vector>

class PBREngine : public GLEngine {
public:
    void init_resources();
    void run();

    void createSphere();
    void createIrradianceMap();
    void createPrefilter();

    void drawModels(Shader& shader, bool skipTextures);

private:
    AllocatedBuffer sphereBuffer;
    int indexCount;
    unsigned int albedoMap, aoMap, normalMap, metallicMap, roughnessMap;

    AllocatedBuffer cubemapBuffer;
    AllocatedBuffer quadBuffer;

    unsigned int hdrTexture;
    unsigned int cubemapTexture, irradianceMap;
    unsigned int prefilterMap;
    unsigned int brdfLUTTexture;

    unsigned int captureFBO, captureRBO, irradianceRBO;
    bool mapInitialized = false;
    bool showModel = false;

    std::vector<glm::vec3> lightPositions;
    std::vector<glm::vec3> lightColors;

    Shader pipeline;
    Shader backgroundPipeline;

    Shader convertToCubemapPipeline;
    Shader createIrradiancePipeline;
    Shader prefilterPipeline;
    Shader brdfPipeline;
};