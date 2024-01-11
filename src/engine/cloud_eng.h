#pragma once

#include "gl_base_engine.h"
#include "utils/gl_compute.h"

class CloudEngine : public GLEngine {
public:
    void init_resources();
    void render(std::vector<Model>& objs);
    void handleImGui();

private:
    void createWorleyNoiseInfo();
    void createWorleyNoiseTexture();

    int cellsPerAxis = 64;

    unsigned int worleyNoiseSSBO;
    unsigned int worleyNoiseTexture;
    glm::vec3 worleyNoiseDimensions = glm::vec3(128, 128, 128);
    float persistence = 1.0f, layer = 0.0f;

    ComputeShader worleyNoiseCompute;
    Shader worleyNoiseShader;

    ScreenQuad screenQuad;

    AllocatedBuffer cubeBuffer;
    glm::mat4 boxModelMatrix = glm::mat4(1.0f);

    glm::vec3 cloudColor = glm::vec3(1.0f);
    float cloudScale = 1.0f, cloudOffset = 0.0f;
    float densityMultiplier = 1.0f, densityThreshold = 0.1f;
    int numSteps = 10;

    glm::vec3 lightPos = glm::vec3(2.0f);
    glm::vec4 phaseParams = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
    float lightAbsorptionTowardsSun = 0.2f, lightAbsorptionThroughCloud = 0.2f;
    float darknessThreshold = 0.5f;
    int numStepsLight = 5;
};