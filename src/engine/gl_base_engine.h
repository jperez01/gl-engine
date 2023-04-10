#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <SDL.h>
#include <vector>

#include "utils/gl_types.h"
#include "utils/shader.h"
#include "utils/camera.h"
#include "utils/gl_model.h"
#include "utils/gl_funcs.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_stdlib.h"
#include "ImGuizmo.h"

struct EnviornmentCubemap {
    AllocatedBuffer buffer;
    unsigned int texture;
    Shader pipeline;

    EnviornmentCubemap() {}

    EnviornmentCubemap(std::string path) {
        pipeline = Shader("cubemap/map.vs", "cubemap/map.fs");

        buffer = glutil::createUnitCube();
        texture = glutil::loadCubemap(path);
    }

    void draw(glm::mat4 &projection, glm::mat4 &view) {
        glm::mat4 convertedView = glm::mat4(glm::mat3(view));
        glDepthFunc(GL_LEQUAL);
            pipeline.use();
            pipeline.setMat4("projection", projection);
            pipeline.setMat4("view", convertedView);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
            pipeline.setInt("skybox", 0);

            glBindVertexArray(buffer.VAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        glDepthFunc(GL_LESS);
    }
};

struct ScreenQuad {
    AllocatedBuffer buffer;

    ScreenQuad() {
        buffer = glutil::createScreenQuad();
    }

    void draw() {
        glBindVertexArray(buffer.VAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
};

class GLEngine {
    public:
        void init();
        virtual void init_resources();
        virtual void run();
        void handleImGui();
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
        ImGuizmo::OPERATION operation = ImGuizmo::OPERATION::TRANSLATE;
        float shininess = 200.0f;

        Camera camera;
        bool handleMouseMovement = true;

        float animationTime = 0.0f;
        int chosenAnimation = 0;

        void handleBasicRenderLoop();
        void handleEvents();
        void handleImportedObjs();

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