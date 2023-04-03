#pragma once

#include "gl_base_engine.h"

struct SimpleDirLight {
    glm::vec3 direction;
    glm::vec3 color;
};
struct SimplePointLight {
    glm::vec3 position;
    glm::vec3 color;
};

class VoxelEngine : public GLEngine {
    public:
        void init_resources();
        void run();

        void createVoxelGrid();
    
    private:
        unsigned int voxelGridTexture;
        float voxelSize, voxelWorldSize, maxCoord, maxDistance = 10.0f;
        float indirectLightMultiplier = 0.5f, dirLightMultiplier = 0.4f,
            specularAngleMultiplier = 0.1f;
        int gridSize = 256;
        float someLod = 0;

        bool shouldShowShadowMap = false;
        unsigned int shadowMapFBO;
        unsigned int shadowDepthTexture, shadowResolution = 2048;
        Shader shadowMapPipeline;

        AllocatedBuffer quadBuffer;
        Shader quadPipeline;

        AllocatedBuffer cubemapBuffer;
        unsigned int cubemapTexture;
        Shader cubemapPipeline;

        glm::mat4 finalVoxelProjection;

        std::vector<SimplePointLight> pointLights;
        SimpleDirLight directionalLight;

        Shader voxelGridPipeline, renderPassPipeline;
};