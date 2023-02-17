#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <SDL.h>
#include <vector>

#include "utils/gl_types.h"
#include "utils/shader.h"
#include "utils/camera.h"
#include "utils/gl_model.h"
#include "utils/gl_compute.h"

class RenderEngine {
    public:
        void init();
        void init_resources();
        void run();
        void draw();
        void cleanup();
    
    private:
        SDL_Window* window;
        SDL_GLContext gl_context;
        int WINDOW_WIDTH = 1280, WINDOW_HEIGHT = 720;

        float lastX = WINDOW_WIDTH / 2.0f;
        float lastY = WINDOW_HEIGHT / 2.0f;
        bool firstMouse = true;

        float deltaTime = 0.0f;
        float lastFrame = 0.0f;
        float animationTime = 0.0f;

        std::vector<Model> importedObjs;
        std::vector<Model> usableObjs;

        int chosenObjIndex = 0;
        int chosenAnimation = 0;
        Camera camera;

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

        void mouse_callback(double xpos, double ypos);
        void scroll_callback(double yoffset);
        void framebuffer_callback(int width, int height);

        void handleClick(double xpos, double ypos);
        void checkIntersection(glm::vec4& origin, glm::vec4& direction, glm::vec4& inverse_dir);

        void async_load_model(std::string path);

        void loadModelData(Model& model);

        void drawModels(Shader& shader, bool skipTextures = false);
        void drawPlane();
};