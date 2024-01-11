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

#include "ui/editor.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_stdlib.h"
#include "ImGuizmo.h"

enum DrawOptions {
    SKIP_TEXTURES = (1u << 0),
    SKIP_CULLING = (1u << 1)
};

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

    ScreenQuad() {}

    void init() {
        buffer = glutil::createScreenQuad();
    }

    void draw() {
        glBindVertexArray(buffer.VAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
};

class GLEngine {
    public:
        virtual void init_resources();
        virtual void render(std::vector<Model> &objs) = 0;
        virtual void handleImGui() = 0;
        virtual void handleObjs(std::vector<Model> &objs) {}

        void loadModelData(Model& model);

        Camera* camera = nullptr;
        int WINDOW_WIDTH = 1920, WINDOW_HEIGHT = 1080;
    
    protected:
        float shininess = 200.0f;

        float animationTime = 0.0f;
        int chosenAnimation = 0;

        void drawModels(std::vector<Model> &models, Shader& shader, unsigned char drawOptions = 0);
        void drawPlane();
};