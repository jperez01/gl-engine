#include "gl_clustered_engine.h"
#include "utils/gl_funcs.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_stdlib.h"
#include "ImGuizmo.h"

#include "glm/gtx/string_cast.hpp"
#include <random>

void ClusteredEngine::init_resources() {
    camera = Camera(glm::vec3(0.0f, 5.0f, 5.0f));

    tileCreateCompute = ComputeShader("clustered/tileCreate.comp");
    clusterLightCompute = ComputeShader("clustered/clusterLights.comp");
    renderPipeline = Shader("clustered/lighting.vs", "clustered/pbr.fs");
    gBufferPipeline = Shader("deferred/gbuffer.vs", "deferred/gbuffer.fs");
    lightBoxPipeline = Shader("deferred/lightBox.vs", "deferred/lightBox.fs");

    Model newModel("../../resources/objects/sponzaBasic/glTF/Sponza.gltf", GLTF);
    loadModelData(newModel);
    usableObjs.push_back(newModel);

    quadBuffer = glutil::createScreenQuad();
    cubeBuffer = glutil::createUnitCube();

    directionalLight.direction = glm::vec3(0.0f, 1.0f, 0.0f);
    directionalLight.diffuse = glm::vec3(0.5f, 0.5f, 0.5f);

    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<float> color_dist(0.5f, 1.0f);
    std::uniform_real_distribution<float> pos_dist(-50.f, 50.f);
    std::uniform_real_distribution<float> posy_dist(0.0f, 100.0f);
    std::uniform_real_distribution<float> posz_dist(-50.f, 50.f);
    std::uniform_real_distribution<float> radius_dist(15.0f, 35.0f);

    for (unsigned i = 0; i < numLights; i++) {
        ClusteredLight light;
        light.color = glm::vec4(color_dist(mt), color_dist(mt), color_dist(mt), 1.0f);
        light.position = glm::vec4(pos_dist(mt), posy_dist(mt), posz_dist(mt), 1.0f);
        light.enabled = 1;
        light.intensity = 0.1f;
        light.range = 40.0f;

        lights.push_back(light);
    }
    float value = log2(camera.zFar / camera.zNear);
    scale = gridSizeZ / value;
    bias = gridSizeZ * log2(camera.zNear) / value;

    glCreateFramebuffers(1, &deferredFBO);

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

    init_SSBOs();
}

void ClusteredEngine::init_SSBOs() {
    unsigned int sizeX = (unsigned int) std::ceilf(WINDOW_WIDTH / (float)gridSizeX);
    unsigned int sizeY = (unsigned int) std::ceilf(WINDOW_HEIGHT / (float)gridSizeY);
    glCreateBuffers(1, &AABBGridSSBO);
    glNamedBufferData(AABBGridSSBO, sizeof(glm::vec4) * 2 * numClusters, nullptr, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, AABBGridSSBO);

    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)WINDOW_WIDTH/ (float)WINDOW_HEIGHT, 0.1f, 100.0f);

    ScreenToView info;
    info.inverseProj = glm::inverse(projection);
    info.screenDimensions = glm::uvec2(WINDOW_WIDTH, WINDOW_HEIGHT);
    info.tileSizes = glm::uvec4(gridSizeX, gridSizeY, gridSizeZ, sizeX);
    info.tileScreenSizes = glm::uvec2(sizeX, sizeY);
    glCreateBuffers(1, &screenToViewSSBO);
    glNamedBufferData(screenToViewSSBO, sizeof(ScreenToView), &info, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, screenToViewSSBO);

    glCreateBuffers(1, &lightSSBO);
    glNamedBufferData(lightSSBO, maxLights * sizeof(ClusteredLight), lights.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, lightSSBO);

    unsigned int totalNumLights = numClusters * maxLightsPerTile;
    glCreateBuffers(1, &lightIndicesSSBO);
    glNamedBufferData(lightIndicesSSBO, totalNumLights * sizeof(unsigned int), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, lightIndicesSSBO);

    glCreateBuffers(1, &lightGridSSBO);
    glNamedBufferData(lightGridSSBO, numClusters * 2 * sizeof(unsigned int), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, lightGridSSBO);

    glCreateBuffers(1, &lightGlobalCountSSBO);
    glNamedBufferData(lightGlobalCountSSBO, sizeof(unsigned int), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, lightGlobalCountSSBO);
}

void ClusteredEngine::run() {
    while(!closedWindow) {
        handleBasicRenderLoop();

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)WINDOW_WIDTH/ (float)WINDOW_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.getViewMatrix();

        tileCreateCompute.use();
        tileCreateCompute.setFloat("zNear", camera.zNear);
        tileCreateCompute.setFloat("zFar", camera.zFar);
        glDispatchCompute(gridSizeX, gridSizeY, gridSizeZ);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        clusterLightCompute.use();
        clusterLightCompute.setMat4("view", view);
        glDispatchCompute(1, 1, 6);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(0.1f));

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderPipeline.use();

        renderPipeline.setFloat("zNear", camera.zNear);
        renderPipeline.setFloat("zFar", camera.zFar);
        renderPipeline.setFloat("bias", bias);
        renderPipeline.setFloat("scale", scale);
        renderPipeline.setVec3("viewPos", camera.Position);
        renderPipeline.setFloat("multiplier", lightMultiplier);
        
        renderPipeline.setVec3("dirLight.direction", directionalLight.direction);
        renderPipeline.setVec3("dirLight.color", directionalLight.diffuse);

        renderPipeline.setMat4("view", view);
        renderPipeline.setMat4("projection", projection);
        renderPipeline.setBool("useFragNormalFunction", shouldUseFragFunction);
        usableObjs[0].model_matrix = model;

        drawModels(renderPipeline);
        
        /*
        glBindVertexArray(quadBuffer.VAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, deferredFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        */

        lightBoxPipeline.use();
        lightBoxPipeline.setMat4("projection", projection);
        lightBoxPipeline.setMat4("view", view);

        for (unsigned int i = 0; i < lights.size(); i++) {
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(lights[i].position));
            model = glm::scale(model, glm::vec3(0.25f));
            lightBoxPipeline.setMat4("model", model);
            lightBoxPipeline.setVec3("lightColor", lights[i].color);

            glBindVertexArray(cubeBuffer.VAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        handleImGui();

        SDL_GL_SwapWindow(window);
    }
}

void ClusteredEngine::handleImGui()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

    ImGui::Begin("Info");
    if (ImGui::CollapsingHeader("Scene Info")) {
        ImGui::SliderFloat("Bias", &bias, 0.01f, 1.0f);
        ImGui::SliderFloat("Scale", &scale, 0.5f, 10.0f);
        ImGui::SliderFloat("Multiplier ", &lightMultiplier, 1.0f, 100.0f);
        ImGui::Checkbox("Should use Frag Function", &shouldUseFragFunction);
    }
    if (ImGui::CollapsingHeader("Directional Light Info")) {
        ImGui::SliderFloat3("Direction", (float*)&directionalLight.direction, -1.0f, 1.0f);
        ImGui::SliderFloat3("Color", (float*)&directionalLight.diffuse, 0.0f, 1.0f);
    }
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
