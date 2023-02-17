#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <SDL.h>
#include <vector>

#include "utils/gl_compute.h"
#include "gl_base_engine.h"

class RenderEngine : public GLEngine {
    public:
        void init_resources();
        void run();
    
    private:
        float animationTime = 0.0f;
        int chosenAnimation = 0;

        AllocatedBuffer planeBuffer;
        unsigned int planeTexture;

        AllocatedBuffer cubemapBuffer;
        unsigned int cubemapTexture;

        AllocatedBuffer quadBuffer;
        
        unsigned int depthFBO;
        unsigned int depthMap;

        unsigned int depthCubemaps[4];
        int shadowHeight = 2048, shadowWidth = 2048;

        unsigned int imgTexture;
        int imgHeight = 1000, imgWidth = 1000;
        ComputeShader computePipeline;

        Shader pipeline;
        Shader mapPipeline;
        Shader shadowmapPipeline;
        Shader depthCubemapPipeline;

        PointLight pointLights[4];
        DirLight directionLight;

        void loadModelData(Model& model);

        void drawModels(Shader& shader, bool skipTextures = false);
        void drawPlane();
};