#pragma once
#include "gl_base_engine.h"
#include "utils/gl_compute.h"

class ComputeEngine : public GLEngine {
public:
    void init_resources();
    void run();

    void createValues();

private:
    std::vector<Sphere> spheres;
    Plane plane;
    std::vector<PointLight> pointLights;
    DirLight dirLight;

    int numReflections = 2;

    unsigned int cubemapTexture;
    
    unsigned int imgTexture;
    int imgHeight = 2048, imgWidth = 2048;
    ComputeShader computePipeline;

    AllocatedBuffer quadBuffer;
    Shader renderPipeline;
};