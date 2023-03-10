#include "gl_deferred_eng.h"
#include "utils/gl_funcs.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_stdlib.h"
#include "ImGuizmo.h"

#include "glm/gtx/string_cast.hpp"
#include <random>

void DeferredEngine::init_resources() {
    camera = Camera(glm::vec3(0.0f, 0.0f, 5.0f));

    renderPipeline = Shader("../../shaders/deferred/lighting.vs", "../../shaders/ssr/finalPassF.glsl");
    gbufferPipeline = Shader("../../shaders/deferred/gbuffer.vs", "../../shaders/deferred/gbuffer.fs");
    ssrPipeline = Shader("../../shaders/deferred/lighting.vs", "../../shaders/ssr/ssrF.glsl");
    
    Model newModel("../../resources/objects/backpack/backpack.obj");
    loadModelData(newModel);
    usableObjs.push_back(newModel);

    quadBuffer = glutil::createScreenQuad();

    planeBuffer = glutil::createPlane();
    planeTexture = glutil::loadTexture("../../resources/textures/wood.png");

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
    gReflectionColor = glutil::createTexture(WINDOW_WIDTH, WINDOW_HEIGHT, GL_FLOAT, GL_RGBA, GL_RGBA16F);
    gReflectionPosition = glutil::createTexture(WINDOW_WIDTH, WINDOW_HEIGHT, GL_FLOAT, GL_RGBA, GL_RGBA16F);

    depthMap = glutil::createTexture(WINDOW_WIDTH, WINDOW_HEIGHT, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, GL_DEPTH_STENCIL, GL_DEPTH32F_STENCIL8);
    
    glNamedFramebufferTexture(deferredFBO, GL_COLOR_ATTACHMENT0, gPosition, 0);
    glNamedFramebufferTexture(deferredFBO, GL_COLOR_ATTACHMENT1, gNormal, 0);
    glNamedFramebufferTexture(deferredFBO, GL_COLOR_ATTACHMENT2, gAlbedo, 0);
    glNamedFramebufferTexture(deferredFBO, GL_COLOR_ATTACHMENT3, gReflectionPosition, 0);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthMap, 0);

    glNamedFramebufferTexture(deferredFBO, GL_DEPTH_STENCIL_ATTACHMENT, depthMap, 0);

    unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
    glNamedFramebufferDrawBuffers(deferredFBO, 4, attachments);

    unsigned int rboDepth;
    // glCreateRenderbuffers(1, &rboDepth);
    // glNamedRenderbufferStorage(rboDepth, GL_DEPTH_COMPONENT, WINDOW_WIDTH, WINDOW_HEIGHT);
    // glNamedFramebufferRenderbuffer(deferredFBO, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    
    if (glCheckNamedFramebufferStatus(deferredFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "Framebuffer not complete! " << std::endl;
    }
    glBindFramebuffer(deferredFBO, 0);

    glCreateFramebuffers(1, &ssrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssrFBO);

    glNamedFramebufferTexture(ssrFBO, GL_COLOR_ATTACHMENT0, gReflectionColor, 0);
    glNamedFramebufferDrawBuffer(ssrFBO, GL_COLOR_ATTACHMENT0);

    unsigned int ssrDepth = glutil::createTexture(WINDOW_WIDTH, WINDOW_HEIGHT, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, GL_DEPTH_STENCIL, GL_DEPTH32F_STENCIL8);
    glNamedFramebufferTexture(ssrFBO, GL_DEPTH_STENCIL_ATTACHMENT, ssrDepth, 0);

    // glCreateRenderbuffers(1, &ssrDepth);
    // glNamedRenderbufferStorage(ssrDepth, GL_DEPTH_COMPONENT, WINDOW_WIDTH, WINDOW_HEIGHT);
    // glNamedFramebufferRenderbuffer(ssrDepth, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, ssrDepth);

    if (glCheckNamedFramebufferStatus(ssrFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "Framebuffer not complete! " << std::endl;
    }
}


void DeferredEngine::run() {
    int shininess = 10;
    float multiplier = 0.01;
    glm::mat4 planeModel = glm::mat4(1.0f);
    planeModel = glm::translate(planeModel, glm::vec3(0.0, 0.0, 0.0));
    planeModel = glm::scale(planeModel, glm::vec3(1.0f, 1.5f, 1.0f));

    ImGuizmo::OPERATION operation = ImGuizmo::OPERATION::TRANSLATE;

    while (!closedWindow) {
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        float currentFrame = static_cast<float>(SDL_GetTicks());
        deltaTime = currentFrame - lastFrame;
        deltaTime *= multiplier;
        lastFrame = currentFrame;

        handleEvents();
        
        if (importedObjs.size() != 0) {
            Model model = importedObjs[0];
            loadModelData(model);

            importedObjs.pop_back();
            usableObjs.push_back(model);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();

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

            if(ImGui::RadioButton("Translate", operation == ImGuizmo::TRANSLATE)) {
                operation = ImGuizmo::TRANSLATE;
            }
            if(ImGui::RadioButton("Rotate", operation == ImGuizmo::ROTATE)) {
                operation = ImGuizmo::ROTATE;
            }
            if(ImGui::RadioButton("Scale", operation == ImGuizmo::SCALE)) {
                operation = ImGuizmo::SCALE;
            }
        }
        ImGui::End();

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)WINDOW_WIDTH/ (float)WINDOW_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);

        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
        ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), operation,
            ImGuizmo::LOCAL, glm::value_ptr(planeModel));

        glBindFramebuffer(GL_FRAMEBUFFER, deferredFBO);
            glClearColor(0.0, 0.0, 0.0, 1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            gbufferPipeline.use();
            gbufferPipeline.setMat4("projection", projection);
            gbufferPipeline.setMat4("view", view);

            glBindTextureUnit(0, planeTexture);
            gbufferPipeline.setInt("texture_diffuse1", 0);

            gbufferPipeline.setMat4("model", planeModel);

            glBindVertexArray(planeBuffer.VAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            for (unsigned int i = 0; i < objectPositions.size(); i++) {
                model = glm::mat4(1.0f);
                model = glm::translate(model, objectPositions[i]);
                model = glm::scale(model, glm::vec3(0.1f));
                usableObjs[0].model_matrix = model;

                gbufferPipeline.setMat4("model", model);
                drawModels(gbufferPipeline);
            }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        glBindFramebuffer(GL_FRAMEBUFFER, ssrFBO);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            ssrPipeline.use();

            glBindTextureUnit(0, depthMap);
            glBindTextureUnit(1, gNormal);
            glBindTextureUnit(2, gAlbedo);
            glBindTextureUnit(3, gReflectionPosition);

            ssrPipeline.setInt("normalTexture", 1);
            ssrPipeline.setInt("colorTexture", 2);
            ssrPipeline.setInt("depthTexture", 0);
            ssrPipeline.setInt("positionTexture", 3);

            ssrPipeline.setMat4("projection", projection);
            ssrPipeline.setMat4("view", view);
            ssrPipeline.setMat4("invProjection", glm::inverse(projection));
            ssrPipeline.setMat4("invView", glm::inverse(view));

            glBindVertexArray(quadBuffer.VAO);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderPipeline.use();

        glBindTextureUnit(0, gPosition);
        glBindTextureUnit(1, gNormal);
        glBindTextureUnit(2, gAlbedo);
        glBindTextureUnit(3, gReflectionColor);

        renderPipeline.setInt("gPosition", 0);
        renderPipeline.setInt("gNormal", 1);
        renderPipeline.setInt("gAlbedoSpec", 2);
        renderPipeline.setInt("gReflectionColor", 3);

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