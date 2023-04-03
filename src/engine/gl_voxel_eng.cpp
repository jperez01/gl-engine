#include "gl_voxel_eng.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_stdlib.h"

#include "glm/gtx/component_wise.hpp"

void VoxelEngine::init_resources() {
    camera = Camera(glm::vec3(0.0f, 0.0f, 10.0f));

    voxelGridPipeline = Shader("coneTracing/voxel.vs", "coneTracing/voxel.fs", "coneTracing/voxel.gs");
    renderPassPipeline = Shader("coneTracing/colorPass.vs", "coneTracing/colorPass.fs");
    shadowMapPipeline = Shader("shadows/map.vs", "shadows/map.fs");
    quadPipeline = Shader("shadows/debug.vs", "shadows/debug.fs");
    cubemapPipeline = Shader("cubemap/map.vs", "cubemap/map.fs");

    Model newModel("../../resources/objects/sponzaBasic/glTF/Sponza.gltf", GLTF);
    newModel.model_matrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.05f));
    loadModelData(newModel);
    usableObjs.push_back(newModel);

    voxelGridTexture = glutil::createTexture3D(gridSize, gridSize, gridSize);
    glBindImageTexture(0, voxelGridTexture, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);

    glCreateFramebuffers(1, &shadowMapFBO);
    shadowDepthTexture = glutil::createTexture(shadowResolution, shadowResolution, 
        GL_FLOAT, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT32F);

    glNamedFramebufferTexture(shadowMapFBO, GL_DEPTH_ATTACHMENT, shadowDepthTexture, 0);
    glNamedFramebufferDrawBuffer(shadowMapFBO, GL_NONE);
    glNamedFramebufferReadBuffer(shadowMapFBO, GL_NONE);

    int status = glCheckNamedFramebufferStatus(shadowMapFBO, GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "Framebuffer is not complete!" << std::endl;
    }

    quadBuffer = glutil::createScreenQuad();

    cubemapBuffer = glutil::createUnitCube();
    std::string cubemapPath = "../../resources/textures/skybox/";
    cubemapTexture = glutil::loadCubemap(cubemapPath);

    directionalLight.direction = glm::vec3(0.0f, -1.0f, 0.0f);
    directionalLight.color = glm::vec3(0.2f);

    glm::vec3 pointLightPositions[] = {
        glm::vec3( 0.7f,  2.0f,  5.0f),
        glm::vec3( 2.3f, -3.3f, -4.0f),
        glm::vec3(-4.0f,  2.0f, -12.0f),
        glm::vec3( 0.0f,  0.0f, -3.0f)
    };

    for (int i = 0; i < 4; i++) {
        SimplePointLight newLight;
        newLight.color = glm::vec3(0.05f * i, 0.05f * i, 0.05f * i);
        newLight.position = pointLightPositions[i];

        pointLights.push_back(newLight);
    }

    voxelSize = 1.0f / gridSize;
    glm::vec4 midpoint = (usableObjs[0].aabb.maxPoint + usableObjs[0].aabb.minPoint) * 0.5f;
    glm::vec4 difference = usableObjs[0].model_matrix * (midpoint - usableObjs[0].aabb.maxPoint);
    maxCoord = glm::compMax(glm::abs(difference)) + 1.0f;
    voxelWorldSize = maxCoord * 2.0f / gridSize;
}

void VoxelEngine::run() {
    float shininess = 200.0f;
    glm::vec3 lightPos(-2.0f, 4.0f, -1.0f);

    createVoxelGrid();

    while (!closedWindow) {
        float currentFrame = static_cast<float>(SDL_GetTicks());
        deltaTime = currentFrame - lastFrame;
        deltaTime *= 0.01f;
        lastFrame = currentFrame;

        handleEvents();

        if (importedObjs.size() != 0) {
            Model model = importedObjs[0];
            loadModelData(model);

            importedObjs.pop_back();
            usableObjs.push_back(model);
        }

        glClearColor(0.0, 0.2, 0.2, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Info");
        if (ImGui::CollapsingHeader("Global Illumination Info")) {
            if (ImGui::SliderFloat("Voxel World Size", &voxelWorldSize, 0.1f, 2.0f)) {
                createVoxelGrid();
            }
            ImGui::SliderFloat("Max Distance", &maxDistance, 0.0f, 20.0f);
            
            ImGui::SliderFloat("Specular Angle Multiplier", &specularAngleMultiplier, 0.0f, 1.0f);
            ImGui::SliderFloat("Indirect Light Multiplier", &indirectLightMultiplier, 0.0f, 1.0f);
        }
        if (ImGui::CollapsingHeader("Directional Light Info")) {
            ImGui::SliderFloat3("Direction", (float*)&directionalLight.direction, -1.0f, 1.0f);
            ImGui::ColorEdit3("Color", (float*)&directionalLight.color);
            ImGui::SliderFloat("Light Multiplier", &dirLightMultiplier, 0.0f, 2.0f);
            
            ImGui::Checkbox("Should show shadow map", &shouldShowShadowMap);
            ImGui::SliderFloat3("Light Position for Shadows", (float*)&lightPos, -10.0f, 10.0f);
        }
        ImGui::End();
        directionalLight.direction = glm::normalize(directionalLight.direction);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)WINDOW_WIDTH/ (float)WINDOW_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.getViewMatrix();

        // Create shadowmap
        float near_plane = 1.0f, far_plane = 100.0f;
        glm::mat4 lightProjection = glm::ortho(-200.0f, 200.0f, -200.0f, 200.0f, near_plane, far_plane);
        glm::mat4 lightView = glm::lookAt(lightPos, lightPos + directionalLight.direction, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;
        
        shadowMapPipeline.use();
        shadowMapPipeline.setMat4("lightSpaceMatrix", lightSpaceMatrix);

        glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
            glViewport(0, 0, shadowResolution, shadowResolution);
            glClear(GL_DEPTH_BUFFER_BIT);

            glCullFace(GL_FRONT);
            drawModels(shadowMapPipeline);
            glCullFace(GL_BACK);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        // Final Render Pass
        glBindTextureUnit(7, shadowDepthTexture);

        renderPassPipeline.use();

        renderPassPipeline.setInt("shadowMap", 7);
        renderPassPipeline.setMat4("lightSpaceMatrix", lightSpaceMatrix);
        renderPassPipeline.setVec3("dirLightPos", lightPos);

        renderPassPipeline.setMat4("view", view);
        renderPassPipeline.setMat4("projection", projection);
        renderPassPipeline.setFloat("shininess", shininess);
        renderPassPipeline.setVec3("viewPos", camera.Position);

        renderPassPipeline.setFloat("VOXEL_SIZE", voxelSize);
        renderPassPipeline.setFloat("MAX_DIST", maxDistance);

        renderPassPipeline.setFloat("dirLightMultiplier", dirLightMultiplier);
        renderPassPipeline.setFloat("indirectLightMultiplier", indirectLightMultiplier);
        renderPassPipeline.setFloat("specularAngleMultiplier", specularAngleMultiplier);
        renderPassPipeline.setInt("gridSize", gridSize);
        renderPassPipeline.setFloat("voxelWorldSize", voxelWorldSize);
        renderPassPipeline.setMat4("voxelProjection", finalVoxelProjection);
        renderPassPipeline.setFloat("someLod", someLod);

        renderPassPipeline.setVec3("dirLight.color", directionalLight.color);
        renderPassPipeline.setVec3("dirLight.direction", directionalLight.direction);
        for (unsigned int i = 0; i < pointLights.size(); i++) {
            renderPassPipeline.setVec3("pointLights[" + std::to_string(i) + "].position", pointLights[i].position);
            renderPassPipeline.setVec3("pointLights[" + std::to_string(i) + "].color", pointLights[i].color);
        }
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_3D, voxelGridTexture);

        renderPassPipeline.setInt("voxelTexture", 8);
        drawModels(renderPassPipeline);

        //Show Debug Shadowmap
        if (shouldShowShadowMap) {
            quadPipeline.use();
            quadPipeline.setInt("depthMap", 7);
            quadPipeline.setFloat("near_plane", near_plane);
            quadPipeline.setFloat("far_plane", far_plane);
            glBindVertexArray(quadBuffer.VAO);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

        // Render Cubemap
        glDepthFunc(GL_LEQUAL);
            glm::mat4 convertedView = glm::mat4(glm::mat3(view));
            cubemapPipeline.use();
            cubemapPipeline.setMat4("projection", projection);
            cubemapPipeline.setMat4("view", convertedView);
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
            cubemapPipeline.setInt("skybox", 0);

            glBindVertexArray(cubemapBuffer.VAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        glDepthFunc(GL_LESS);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }
}

void VoxelEngine::createVoxelGrid() {
    glm::mat4 voxelProjection = glm::ortho(-maxCoord, maxCoord, -maxCoord, maxCoord, 0.1f, 2.0f * maxCoord + 0.1f);

    glm::mat4 finalProjection = voxelProjection * glm::lookAt(glm::vec3(0, 0, maxCoord + 0.1f), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    finalVoxelProjection = finalProjection;

    glViewport(0, 0, gridSize, gridSize);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

        // Voxel Grid Color Data
        voxelGridPipeline.use();

        voxelGridPipeline.setMat4("projection", finalProjection);

        voxelGridPipeline.setVec3("dirLight.color", directionalLight.color);
        voxelGridPipeline.setVec3("dirLight.direction", directionalLight.direction);
        for (unsigned int i = 0; i < pointLights.size(); i++) {
            voxelGridPipeline.setVec3("pointLights[" + std::to_string(i) + "].position", pointLights[i].position);
            voxelGridPipeline.setVec3("pointLights[" + std::to_string(i) + "].color", pointLights[i].color);
        }
        voxelGridPipeline.setInt("gridSize", gridSize);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, voxelGridTexture);
        voxelGridPipeline.setInt("voxelTexture", 0);
        glBindImageTexture(0, voxelGridTexture, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);

        drawModels(voxelGridPipeline);
        glGenerateTextureMipmap(voxelGridTexture);
        
        
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
}