#include "gl_deferred_eng.h"
#include "gl_funcs.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_stdlib.h"

#include "glm/gtx/string_cast.hpp"
#include <random>

void DeferredEngine::init_resources() {
    camera = Camera(glm::vec3(0.0f, 0.0f, 5.0f));

    renderPipeline = Shader("../../shaders/deferred/lighting.vs", "../../shaders/deferred/lighting.fs");
    gbufferPipeline = Shader("../../shaders/deferred/gbuffer.vs", "../../shaders/deferred/gbuffer.fs");

    
    Model newModel("../../resources/objects/elf/scene.gltf");
    loadModelData(newModel);
    usableObjs.push_back(newModel);

    quadBuffer = glutil::createScreenQuad();
    objectPositions.push_back(glm::vec3(-3.0, -0.5, -3.0));
    objectPositions.push_back(glm::vec3( 0.0, -0.5, -3.0));
    objectPositions.push_back(glm::vec3( 3.0, -0.5, -3.0));
    objectPositions.push_back(glm::vec3(-3.0, -0.5,  0.0));
    objectPositions.push_back(glm::vec3( 0.0, -0.5,  0.0));
    objectPositions.push_back(glm::vec3( 3.0, -0.5,  0.0));
    objectPositions.push_back(glm::vec3(-3.0, -0.5,  3.0));
    objectPositions.push_back(glm::vec3( 0.0, -0.5,  3.0));
    objectPositions.push_back(glm::vec3( 3.0, -0.5,  3.0));

    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<float> color_dist(0.5f, 1.0f);
    std::uniform_real_distribution<float> posz_dist(-3.f, 3.f);
    std::uniform_real_distribution<float> pos_dist(-3.f, 3.f);

    for (int i = 0; i < 32; i++) {
        DeferredLight light;
        light.color = glm::vec3(color_dist(mt), color_dist(mt), color_dist(mt));
        light.position = glm::vec3(pos_dist(mt), pos_dist(mt), posz_dist(mt));

        lights.push_back(light);
    }

    glCreateFramebuffers(1, &deferredFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, deferredFBO);

    gPosition = glutil::createTexture(WINDOW_WIDTH, WINDOW_HEIGHT, GL_FLOAT, GL_RGBA, GL_RGBA16F, nullptr);
    gNormal = glutil::createTexture(WINDOW_WIDTH, WINDOW_HEIGHT, GL_FLOAT, GL_RGBA, GL_RGBA16F, nullptr);
    gAlbedo = glutil::createTexture(WINDOW_WIDTH, WINDOW_HEIGHT, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA8, nullptr);

    glNamedFramebufferTexture(deferredFBO, GL_COLOR_ATTACHMENT0, gPosition, 0);
    glNamedFramebufferTexture(deferredFBO, GL_COLOR_ATTACHMENT1, gNormal, 0);
    glNamedFramebufferTexture(deferredFBO, GL_COLOR_ATTACHMENT2, gAlbedo, 0);

    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glNamedFramebufferDrawBuffers(deferredFBO, 3, attachments);

    unsigned int rboDepth;
    glCreateRenderbuffers(1, &rboDepth);
    glNamedRenderbufferStorage(rboDepth, GL_DEPTH_COMPONENT, WINDOW_WIDTH, WINDOW_HEIGHT);
    glNamedFramebufferRenderbuffer(deferredFBO, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    
    if (glCheckNamedFramebufferStatus(deferredFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "Framebuffer not complete! " << std::endl;
    }
}


void DeferredEngine::run() {
    bool closedWindow = false;
    SDL_Event event;
    int shininess = 10;
    float multiplier = 0.01;
    bool shouldMove = false;
    SDL_Keycode type;

    bool keyDown[4] = { false, false, false, false};

    while (!closedWindow) {
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        float currentFrame = static_cast<float>(SDL_GetTicks());
        deltaTime = currentFrame - lastFrame;
        deltaTime *= multiplier;
        lastFrame = currentFrame;

        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) {
                closedWindow = true;
            } else if (event.type == SDL_KEYUP) {
                shouldMove = false;
                type = event.key.keysym.sym;
                keyDown[type - SDLK_RIGHT] = false;
            } else if (event.type == SDL_KEYDOWN) {
                shouldMove = true;
                type = event.key.keysym.sym;
                keyDown[type - SDLK_RIGHT] = true;
            } else if (event.type == SDL_MOUSEMOTION && !io.WantCaptureMouse) {
                mouse_callback(event.motion.x, event.motion.y);
            } else if (event.type == SDL_MOUSEWHEEL) {
                scroll_callback(event.wheel.y);
            } else if (event.type == SDL_WINDOWEVENT
                && event.window.event == SDL_WINDOWEVENT_RESIZED) {
                framebuffer_callback(event.window.data1, event.window.data2);
            }
        }

        if (shouldMove) {
            for (int i = 0; i < 4; i++) {
                type = keyDown[i] ? SDLK_RIGHT + i : 0;

                if (type == SDLK_UP)
                    camera.processKeyboard(FORWARD, deltaTime);
                
                if (type == SDLK_DOWN)
                    camera.processKeyboard(BACKWARD, deltaTime);

                if (type == SDLK_LEFT)
                    camera.processKeyboard(LEFT, deltaTime);

                if (type == SDLK_RIGHT)
                    camera.processKeyboard(RIGHT, deltaTime);
            }
        }

        if (importedObjs.size() != 0) {
            Model model = importedObjs[0];
            loadModelData(model);

            importedObjs.pop_back();
            usableObjs.push_back(model);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Info");
        if (ImGui::CollapsingHeader("Lights")) {
            for (int i = 0; i < 32; i++) {
                std::string name = "Light  " + std::to_string(i);
                auto& currentLight = lights[i];
                if (ImGui::TreeNode(name.c_str())) {
                    ImGui::SliderFloat3("Position", (float*)&currentLight.position, -2.0, 2.0);
                    ImGui::SliderFloat3("Color",(float*) &currentLight.color, 0.0, 1.0);
                    ImGui::TreePop();
                }
            }
        }
        if (ImGui::CollapsingHeader("Scene Info")) {
            ImGui::SliderFloat("Camera Multiplier", &multiplier, 0.00001f, 0.01f);
        }
        ImGui::End();

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)WINDOW_WIDTH/ (float)WINDOW_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.getViewMatrix();

        glBindFramebuffer(GL_FRAMEBUFFER, deferredFBO);
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        gbufferPipeline.use();
        gbufferPipeline.setMat4("projection", projection);
        gbufferPipeline.setMat4("view", view);

        glm::mat4 model = glm::mat4(1.0f);
        for (unsigned int i = 0; i < objectPositions.size(); i++) {
            model = glm::mat4(1.0f);
            model = glm::translate(model, objectPositions[i]);
            model = glm::scale(model, glm::vec3(0.01f));

            gbufferPipeline.setMat4("model", model);
            drawModels(gbufferPipeline);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderPipeline.use();

        glBindTextureUnit(0, gPosition);
        glBindTextureUnit(1, gNormal);
        glBindTextureUnit(2, gAlbedo);

        renderPipeline.setInt("gPosition", 0);
        renderPipeline.setInt("gNormal", 1);
        renderPipeline.setInt("gAlbedoSpec", 2);

        for (unsigned int i = 0; i < lights.size(); i++) {
            std::string name = "lights[" + std::to_string(i) + "].";
            renderPipeline.setVec3(name + "Position", lights[i].position);
            renderPipeline.setVec3(name + "Color", lights[i].color);

            const float constant = 1.0f;
            const float linear = 0.7f;
            const float quadratic = 1.8f;

            renderPipeline.setFloat(name + "Linear", linear);
            renderPipeline.setFloat(name + "Quadratic", quadratic);

            const float maxBrightness = std::fmaxf(std::fmaxf(lights[i].color.r, lights[i].color.g), lights[i].color.b);
            float radius = (-linear + std::sqrt(linear * linear - 4 * quadratic * (constant - (256.0f / 5.0f) * maxBrightness))) / (2.0f * quadratic);
            renderPipeline.setFloat(name + "Radius", radius);
        }
        renderPipeline.setVec3("viewPos", camera.Position);
        
        glBindVertexArray(quadBuffer.VAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, deferredFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }
}