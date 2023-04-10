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
        void handleImGui();

        void createVoxelGrid();
    
    private:
        unsigned int voxelGridTexture;
        float voxelSize, voxelWorldSize, maxCoord, maxDistance = 10.0f;
        float indirectLightMultiplier = 0.5f, dirLightMultiplier = 0.4f,
            specularAngleMultiplier = 0.1f;
        int gridSize = 256;
        float someLod = 0;
        bool useAO = true;

        bool shouldShowShadowMap = false, cullFront = false;
        float cameraNearPlane = 0.1f;
        float cameraFarPlane = 200.0f;
        int depthMapResolution = 2048;
        unsigned int shadowMapFBO;
        unsigned int lightDepthMaps;
        unsigned int matricesUBO;
        std::vector<float> shadowCascadeLevels = { cameraFarPlane / 50.0f, 
            cameraFarPlane / 25.0f, cameraFarPlane / 10.0f, cameraFarPlane / 5.0f, cameraFarPlane / 2.0f };
        Shader cascadeMapPipeline;

        AllocatedBuffer quadBuffer;
        Shader quadPipeline;

        EnviornmentCubemap cubemap;

        glm::mat4 finalVoxelProjection;

        std::vector<SimplePointLight> pointLights;
        SimpleDirLight directionalLight;

        Shader voxelGridPipeline, renderPassPipeline;

        std::vector<glm::mat4> getLightSpaceMatrices();
        glm::mat4 getLightSpaceMatrix(float nearPlane, float farPlane);
        std::vector<glm::vec4> getFrustumCornerWorldSpace(const glm::mat4& proj, const glm::mat4& view);
};