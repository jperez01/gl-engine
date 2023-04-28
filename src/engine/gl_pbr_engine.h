#pragma once

#include "gl_base_engine.h"

#include <vector>

class PBREngine : public GLEngine {
public:
    void init_resources();
    void render(std::vector<Model> &objs);
    void handleEvents();
    void handleImGui();

    void createSphere();
    void createIrradianceMap();
    void createPrefilter();

private:
    AllocatedBuffer sphereBuffer;
    int indexCount;
    unsigned int albedoMap, aoMap, normalMap, metallicMap, roughnessMap;

    AllocatedBuffer cubemapBuffer;
    AllocatedBuffer quadBuffer;

    bool switchValues = false;

    unsigned int hdrTexture;
    unsigned int cubemapTexture, irradianceMap;
    unsigned int prefilterMap;
    unsigned int brdfLUTTexture;

    unsigned int captureFBO, captureRBO, irradianceRBO;
    bool mapInitialized = false;
    bool showModel = false;

    std::vector<glm::vec3> lightPositions;
    std::vector<glm::vec3> lightColors;
    DirLight directionLight;

    Shader pipeline;
    Shader backgroundPipeline;

    Shader convertToCubemapPipeline;
    Shader createIrradiancePipeline;
    Shader prefilterPipeline;
    Shader brdfPipeline;
};