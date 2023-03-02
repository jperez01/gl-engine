#include "gl_engine.h"
#include "utils/gl_funcs.h"

#include <iostream>
#include <iterator>
#include <SDL.h>
#include <thread>
#include <future>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_stdlib.h"
#include "ImGuizmo.h"

void RenderEngine::init_resources() {
    camera = Camera(glm::vec3(0.0f, 0.0f, 7.0f));
    
    pipeline = Shader("../../shaders/shadowPoints/model.vs", "../../shaders/shadowPoints/model.fs");
    mapPipeline = Shader("../../shaders/cubemap/map.vs", "../../shaders/cubemap/map.fs");
    cascadeMapPipeline = Shader("../../shaders/shadows/cascadeV.glsl", "../../shaders/shadows/map.fs", "../../shaders/shadows/cascadeG.glsl");
    depthCubemapPipeline = Shader("../../shaders/shadowPoints/map.vs", "../../shaders/shadowPoints/map.fs",
        "../../shaders/shadowPoints/map.gs");
    
    Model newModel("../../resources/objects/backpack/backpack.obj");
    loadModelData(newModel);
    usableObjs.push_back(newModel);

    for (int i = 0; i < 4; i++) {
        depthCubemaps[i] = glutil::createCubemap(2048, 2048, GL_FLOAT, 0);
    }

    glCreateFramebuffers(1, &depthFBO);
    glCreateTextures(GL_TEXTURE_2D, 1, &depthMap);
    glTextureStorage2D(depthMap, 1, GL_DEPTH_COMPONENT32F, 2048, 2048);
    glNamedFramebufferTexture(depthFBO, GL_DEPTH_ATTACHMENT, depthMap, 0);
    glNamedFramebufferDrawBuffer(depthFBO, GL_NONE);
    glNamedFramebufferReadBuffer(depthFBO, GL_NONE);

    planeBuffer = glutil::createPlane();
    planeTexture = glutil::loadTexture("../../resources/textures/wood.png");

    cubemapBuffer = glutil::createUnitCube();

    quadBuffer = glutil::createScreenQuad();

    std::string cubemapPath = "../../resources/textures/skybox/";
    cubemapTexture = glutil::loadCubemap(cubemapPath);

    directionLight.ambient = glm::vec3(0.05f, 0.05f, 0.05f);
    directionLight.specular = glm::vec3(0.5f, 0.5f, 0.5f);
    directionLight.diffuse = glm::vec3(0.4f, 0.4f, 0.4f);
    directionLight.direction = glm::vec3(-0.2f, -1.0f, -0.3f);

    glm::vec3 pointLightPositions[] = {
        glm::vec3( 0.7f,  2.0f,  5.0f),
        glm::vec3( 2.3f, -3.3f, -4.0f),
        glm::vec3(-4.0f,  2.0f, -12.0f),
        glm::vec3( 0.0f,  0.0f, -3.0f)
    };

    for (int i = 0; i < 4; i++) {
        pointLights[i].ambient = glm::vec3(0.05f * i, 0.05f * i, 0.05f * i);
        pointLights[i].specular = glm::vec3(0.05f * i, 0.05f * i, 0.05f * i);
        pointLights[i].diffuse = glm::vec3(0.05f * i, 0.05f * i, 0.05f * i);
        pointLights[i].position = pointLightPositions[i];
    }

    glCreateBuffers(1, &matricesUBO);
    glNamedBufferData(matricesUBO, sizeof(glm::mat4x4) * 16, nullptr, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, matricesUBO);

    lightDepthMaps = glutil::createTextureArray(shadowCascadeLevels.size() + 1, depthMapResolution, depthMapResolution, GL_FLOAT, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT32F);

    glCreateFramebuffers(1, &dirDepthFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, dirDepthFBO);
    glNamedFramebufferTexture(dirDepthFBO, GL_DEPTH_ATTACHMENT, lightDepthMaps, 0);
    glNamedFramebufferDrawBuffer(dirDepthFBO, GL_NONE);
    glNamedFramebufferReadBuffer(dirDepthFBO, GL_NONE);

    int status = glCheckNamedFramebufferStatus(dirDepthFBO, GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "Framebuffer is not complete!" << std::endl;
    }
}

void RenderEngine::run() {
    float shininess = 100.0;
    std::string path = "";
    glm::vec3 translate = glm::vec3(0.0), rotation = glm::vec3(0.0), scale = glm::vec3(1.0);
    ImGuizmo::OPERATION operation = ImGuizmo::OPERATION::TRANSLATE;

    float startTime = static_cast<float>(SDL_GetTicks());

    while (!closedWindow) {
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        float currentFrame = static_cast<float>(SDL_GetTicks());
        deltaTime = currentFrame - lastFrame;
        deltaTime *= 0.1;
        lastFrame = currentFrame;
        animationTime = (currentFrame - startTime) / 1000.0f;

        glClearColor(1.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        handleEvents();

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)WINDOW_WIDTH/ (float)WINDOW_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.getViewMatrix();

        // Cascaded Shadow calculation
        const auto lightMatrices = getLightSpaceMatrices();
        glNamedBufferSubData(matricesUBO, 0, sizeof(glm::mat4x4) * lightMatrices.size(), lightMatrices.data());

        glViewport(0, 0, depthMapResolution, depthMapResolution);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindFramebuffer(GL_FRAMEBUFFER, dirDepthFBO);
            cascadeMapPipeline.use();
            renderScene(cascadeMapPipeline, true);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Shadow Cubemap Calculation
        glViewport(0, 0, 2048, 2048);
        glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
            float aspect = (float)shadowWidth / (float)shadowHeight;
            float near = 1.0f;
            float far = 25.0f;
            glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, near, far);
            glm::mat4 planeModel = glm::mat4(1.0f);
            planeModel = glm::translate(planeModel, glm::vec3(0.0, -2.0, 0.0));

            depthCubemapPipeline.use();

            for (int i = 0; i < 4; i++) {
                glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemaps[i], 0);
                glClear(GL_DEPTH_BUFFER_BIT);

                glm::vec3 lightPos = pointLights[i].position;
                std::vector<glm::mat4> shadowTransforms;
                shadowTransforms.push_back(shadowProj * 
                    glm::lookAt(lightPos, lightPos + glm::vec3( 1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0)));
                shadowTransforms.push_back(shadowProj * 
                    glm::lookAt(lightPos, lightPos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0)));
                shadowTransforms.push_back(shadowProj * 
                    glm::lookAt(lightPos, lightPos + glm::vec3( 0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
                shadowTransforms.push_back(shadowProj * 
                    glm::lookAt(lightPos, lightPos + glm::vec3( 0.0,-1.0, 0.0), glm::vec3(0.0, 0.0,-1.0)));
                shadowTransforms.push_back(shadowProj * 
                    glm::lookAt(lightPos, lightPos + glm::vec3( 0.0, 0.0, 1.0), glm::vec3(0.0,-1.0, 0.0)));
                shadowTransforms.push_back(shadowProj * 
                    glm::lookAt(lightPos, lightPos + glm::vec3( 0.0, 0.0,-1.0), glm::vec3(0.0,-1.0, 0.0)));

                for (int i = 0; i < 6; i++) {
                    depthCubemapPipeline.setMat4("shadowMatrices[" + std::to_string(i)+ "]", shadowTransforms[i]);
                }
                depthCubemapPipeline.setVec3("lightPos", lightPos);
                depthCubemapPipeline.setFloat("far_plane", far);

                drawModels(depthCubemapPipeline, true);

                depthCubemapPipeline.setMat4("model", planeModel);
                glBindVertexArray(planeBuffer.VAO);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();

        ImGui::Begin("Info");
        if (ImGui::CollapsingHeader("Point Lights")) {
            for (int i = 0; i < 4; i++) {
                std::string name = "Point Light " + std::to_string(i);
                auto& currentLight = pointLights[i];
                if (ImGui::TreeNode(name.c_str())) {
                    ImGui::SliderFloat3("Position", (float*)&currentLight.position, -50.0, 50.0);
                    ImGui::SliderFloat3("Ambient", (float*)&currentLight.ambient, 0.0, 1.0);
                    ImGui::SliderFloat3("Specular",(float*) &currentLight.specular, 0.0, 1.0);
                    ImGui::SliderFloat3("Diffuse", (float*)&currentLight.diffuse, 0.0, 1.0);
                    ImGui::TreePop();
                }
            }
        }

        if (ImGui::CollapsingHeader("Directional Light")) {
            ImGui::SliderFloat3("Direction", (float*)&directionLight.direction, -1.0, 1.0);
            ImGui::SliderFloat3("Ambient", (float*)&directionLight.ambient, 0.0, 1.0);
            ImGui::SliderFloat3("Specular", (float*)&directionLight.specular, 0.0, 1.0);
            ImGui::SliderFloat3("Diffuse", (float*)&directionLight.diffuse, 0.0, 1.0);
        }

        if (ImGui::CollapsingHeader("Models")) {
            ImGui::InputText("Model Path", &path);
            if (ImGui::Button("Load Model")) {
                std::string chosenPath = "../../resources/objects/" + path;
                std::thread(&RenderEngine::async_load_model, this, chosenPath).detach();
            }

            if (usableObjs.size() != 0) {
                ImGui::ListBoxHeader("Models");
                for (size_t i = 0; i < usableObjs.size(); i++) {
                    std::string selectName = "Object " + std::to_string(i);
                    if (ImGui::Selectable(selectName.c_str())) {
                        chosenObjIndex = i;
                    }
                }
                ImGui::ListBoxFooter();
                ImGui::SliderInt("Animation", &chosenAnimation, 0, usableObjs[chosenObjIndex].numAnimations-1);
                if (ImGui::SliderFloat3("Translation", (float*)&translate, -10.0, 10.0)
                    || ImGui::SliderFloat3("Rotation", (float*)&rotation, 0.0, 90.0)
                    || ImGui::SliderFloat3("Scale", (float*)&scale, 0.01f, 1.0f)) {
                    glm::mat4 transformation = glm::mat4(1.0f);
                    transformation = glm::translate(transformation, translate);
                    transformation = glm::scale(transformation, scale);

                    usableObjs[chosenObjIndex].model_matrix = transformation;
                }
            }
        }

        if (ImGui::CollapsingHeader("Extras")) {
            ImGui::SliderFloat("Shininess", &shininess, 1, 200);
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

        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
        ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), operation,
            ImGuizmo::LOCAL, glm::value_ptr(usableObjs[chosenObjIndex].model_matrix));

        if (importedObjs.size() != 0) {
            Model model = importedObjs[0];
            loadModelData(model);

            importedObjs.pop_back();
            usableObjs.push_back(model);
        }

        pipeline.use();

        pipeline.setMat4("projection", projection);
        pipeline.setMat4("view", view);
        pipeline.setFloat("shininess", shininess);
        pipeline.setFloat("far_plane", far);

        pipeline.setVec3("dirLight.direction", directionLight.direction);
        pipeline.setVec3("dirLight.ambient", directionLight.ambient);
        pipeline.setVec3("dirLight.specular", directionLight.specular);
        pipeline.setVec3("dirLight.diffuse", directionLight.diffuse);

        for (int i = 0; i < 4; i++) {
            std::string name = "pointLights[" + std::to_string(i) + "]";

            pipeline.setVec3(name + ".position", pointLights[i].position);
            pipeline.setVec3(name + ".ambient", pointLights[i].ambient);
            pipeline.setVec3(name + ".specular", pointLights[i].specular);
            pipeline.setVec3(name + ".diffuse", pointLights[i].diffuse);
        }

        pipeline.setVec3("viewPos", camera.Position);

        glBindTextureUnit(3, depthMap);
        pipeline.setInt("shadowMap", 3);

        pipeline.setInt("cascadeCount", shadowCascadeLevels.size());
        for (size_t i = 0; i < shadowCascadeLevels.size(); i++) {
            pipeline.setFloat("cascadePlaneDistances[" + std::to_string(i) + "]", shadowCascadeLevels[i]);
        }
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D_ARRAY, lightDepthMaps);
        pipeline.setInt("cascadedMap", 8);
        pipeline.setMat4("view", view);

        for (int i = 0; i < 4; i++) {
            glActiveTexture(GL_TEXTURE4 + i);
            glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemaps[i]);
            pipeline.setInt("shadowMaps[" + std::to_string(i) + "]", 4 + i);
        }

        renderScene(pipeline);

        glDepthFunc(GL_LEQUAL);
        glm::mat4 convertedView = glm::mat4(glm::mat3(view));
        mapPipeline.use();
        mapPipeline.setMat4("projection", projection);
        mapPipeline.setMat4("view", convertedView);

        glBindVertexArray(cubemapBuffer.VAO);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glDepthFunc(GL_LESS);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }
}

void RenderEngine::renderScene(Shader& shader, bool skipTextures) {
    drawModels(shader, skipTextures);

    glm::mat4 planeModel = glm::mat4(1.0f);
    planeModel = glm::translate(planeModel, glm::vec3(0.0, -2.0, 0.0));

    if (!skipTextures) {
        glBindTextureUnit(0, planeTexture);
        shader.setInt("diffuseTexture", 0);
    }
    shader.setMat4("model", planeModel);
    glBindVertexArray(planeBuffer.VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

std::vector<glm::mat4> RenderEngine::getLightSpaceMatrices() {
    std::vector<glm::mat4> result;

    for (int i = 0; i < shadowCascadeLevels.size() + 1; i++) {
        if (i == 0)
            result.push_back(getLightSpaceMatrix(cameraNearPlane, shadowCascadeLevels[i]));
        else if (i < shadowCascadeLevels.size())
            result.push_back(getLightSpaceMatrix(shadowCascadeLevels[i-1], shadowCascadeLevels[i]));
        else
            result.push_back(getLightSpaceMatrix(shadowCascadeLevels[i-1], cameraFarPlane));
    }

    return result;
}

glm::mat4 RenderEngine::getLightSpaceMatrix(float nearPlane, float farPlane) {
    const auto proj = glm::perspective(
        glm::radians(camera.Zoom), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, nearPlane,
        farPlane);
    auto corners = getFrustumCornerWorldSpace(proj, camera.getViewMatrix());

    glm::vec3 center(0, 0, 0);
    for (const auto& v : corners) center += glm::vec3(v);
    center /= corners.size();

    const auto lightView = glm::lookAt(center + directionLight.direction, center, glm::vec3(0.0f, 1.0f, 0.0f));

    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();
    for (const auto& v : corners)
    {
        const auto trf = lightView * v;
        minX = std::min(minX, trf.x);
        maxX = std::max(maxX, trf.x);
        minY = std::min(minY, trf.y);
        maxY = std::max(maxY, trf.y);
        minZ = std::min(minZ, trf.z);
        maxZ = std::max(maxZ, trf.z);
    }

    float zMult = 10.0f;
    if (minZ < 0) minZ *= zMult;
    else minZ /= zMult;

    if (maxZ < 0) maxZ /= zMult;
    else maxZ *= zMult;

    const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
    return lightProjection * lightView;
}

std::vector<glm::vec4> RenderEngine::getFrustumCornerWorldSpace(const glm::mat4& proj, const glm::mat4& view) {
    const auto inv = glm::inverse(proj * view);

    std::vector<glm::vec4> frustumCorners;
    for (unsigned int x = 0; x < 2; x++) {
        for (unsigned int y = 0; y < 2; y++) {
            for (unsigned int z = 0; z < 2; z++) {
                const glm::vec4 pt = inv * glm::vec4(
                    2.0f * x - 1.0f,
                    2.0f * y - 1.0f,
                    2.0f * z - 1.0f,
                    1.0f);
                frustumCorners.push_back(pt / pt.w);
            }
        }
    }

    return frustumCorners;
}