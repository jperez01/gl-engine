#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <SDL.h>
#include <vector>

#include "utils/gl_types.h"
#include "utils/shader.h"
#include "utils/camera.h"
#include "utils/gl_model.h"

class GLEngine {
    public:
        void init();
        virtual void init_resources();
        virtual void run();
        void cleanup();
    
    protected:
        SDL_Window* window;
        SDL_GLContext gl_context;
        int WINDOW_WIDTH = 1920, WINDOW_HEIGHT = 1080;

        bool closedWindow = false;
        bool keyDown[4] = { false, false, false, false};

        float lastX = WINDOW_WIDTH / 2.0f;
        float lastY = WINDOW_HEIGHT / 2.0f;
        bool firstMouse = true;

        float deltaTime = 0.0f;
        float lastFrame = 0.0f;

        std::vector<Model> importedObjs;
        std::vector<Model> usableObjs;
        int chosenObjIndex = 0;
        Camera camera;

        float animationTime = 0.0f;
        int chosenAnimation = 0;

        void handleEvents();

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