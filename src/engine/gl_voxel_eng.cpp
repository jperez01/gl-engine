#include "gl_voxel_eng.h"

#include "glm/gtx/component_wise.hpp"

void VoxelEngine::init_resources() {
    camera->zNear = cameraNearPlane;
    camera->zFar = cameraFarPlane;

    voxelGridPipeline = Shader("coneTracing/voxel.vs", "coneTracing/voxel.fs", "coneTracing/voxel.gs");
    renderPassPipeline = Shader("coneTracing/colorPass.vs", "coneTracing/colorPass.fs");
    quadPipeline = Shader("shadows/debug.vs", "shadows/debug.fs");

    voxelGridTexture = glutil::createTexture3D(gridSize, gridSize, gridSize);
    glBindImageTexture(0, voxelGridTexture, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);
        
    quadBuffer = glutil::createScreenQuad();
    std::string cubemapPath = "../../resources/textures/skybox/";
    cubemap = EnviornmentCubemap(cubemapPath);
    
    directionalLight.direction = glm::vec3(0.2f, 1.0f, 0.3f);
    directionalLight.color = glm::vec3(0.8f);

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

    // Create Directional Light Shadow Info
    cascadeMapPipeline = Shader("shadows/cascadeV.glsl", "shadows/map.fs", "shadows/cascadeG.glsl");
    glCreateBuffers(1, &matricesUBO);
    glNamedBufferData(matricesUBO, sizeof(glm::mat4x4) * 16, nullptr, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, matricesUBO);

    glGenTextures(1, &lightDepthMaps);
    glBindTexture(GL_TEXTURE_2D_ARRAY, lightDepthMaps);
    glTexImage3D(
        GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, depthMapResolution, depthMapResolution, int(shadowCascadeLevels.size()) + 1,
        0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glCreateFramebuffers(1, &shadowMapFBO);
    glNamedFramebufferTexture(shadowMapFBO, GL_DEPTH_ATTACHMENT, lightDepthMaps, 0);
    glNamedFramebufferDrawBuffer(shadowMapFBO, GL_NONE);
    glNamedFramebufferReadBuffer(shadowMapFBO, GL_NONE);

    int status = glCheckNamedFramebufferStatus(shadowMapFBO, GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "Framebuffer is not complete!" << std::endl;
    }
}

void VoxelEngine::render(std::vector<Model>& objs) {
    if (firstTime) {
        firstTime = false;
        createVoxelGrid(objs);
    }
    glClearColor(0.0, 0.2, 0.2, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom), (float)WINDOW_WIDTH/ (float)WINDOW_HEIGHT, cameraNearPlane, cameraFarPlane);
    glm::mat4 view = camera->getViewMatrix();

    // Create shadowmap
    auto lightMatrices = getLightSpaceMatrices();
    glNamedBufferSubData(matricesUBO, 0, sizeof(glm::mat4x4) * lightMatrices.size(), lightMatrices.data());

    cascadeMapPipeline.use();

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_CLAMP);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
        glViewport(0, 0, depthMapResolution, depthMapResolution);
        glClear(GL_DEPTH_BUFFER_BIT);

        if (cullFront) glCullFace(GL_FRONT);
        drawModels(objs, cascadeMapPipeline, SKIP_TEXTURES);
        glCullFace(GL_BACK);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_CLAMP);
    glDisable(GL_CULL_FACE);

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    // Final Render Pass
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D_ARRAY, lightDepthMaps);

    renderPassPipeline.use();

    renderPassPipeline.setInt("cascadedMap", 7);
    renderPassPipeline.setInt("cascadeCount", shadowCascadeLevels.size());
    for (size_t i = 0; i < shadowCascadeLevels.size(); i++) {
        renderPassPipeline.setFloat("cascadePlaneDistances[" + std::to_string(i) + "]", shadowCascadeLevels[i]);
    }
    renderPassPipeline.setFloat("far_plane", cameraFarPlane);
    renderPassPipeline.setBool("showShadows", shouldShowShadowMap);

    renderPassPipeline.setMat4("view", view);
    renderPassPipeline.setMat4("projection", projection);
    renderPassPipeline.setFloat("shininess", shininess);
    renderPassPipeline.setVec3("viewPos", camera->Position);
    renderPassPipeline.setBool("useAO", useAO);

    renderPassPipeline.setFloat("dirLightMultiplier", dirLightMultiplier);
    renderPassPipeline.setFloat("indirectLightMultiplier", indirectLightMultiplier);
    renderPassPipeline.setFloat("specularAngleMultiplier", specularAngleMultiplier);

    renderPassPipeline.setFloat("MAX_DIST", maxDistance);
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
    drawModels(objs, renderPassPipeline);

    // Render Cubemap
    cubemap.draw(projection, view);
}

void VoxelEngine::handleImGui() {
    if (ImGui::CollapsingHeader("Global Illumination Info")) {
        if (ImGui::SliderFloat("Voxel World Size", &voxelWorldSize, 0.1f, 2.0f)) {
        }
        ImGui::SliderFloat("Max Distance", &maxDistance, 0.0f, 20.0f);
        ImGui::Checkbox("Use Ambient Occlusion", &useAO);
        
        ImGui::SliderFloat("Specular Angle Multiplier", &specularAngleMultiplier, 0.0f, 1.0f);
        ImGui::SliderFloat("Indirect Light Multiplier", &indirectLightMultiplier, 0.0f, 3.0f);
    }
    if (ImGui::CollapsingHeader("Directional Light Info")) {
        ImGui::SliderFloat3("Direction", (float*)&directionalLight.direction, -1.0f, 1.0f);
        ImGui::ColorEdit3("Color", (float*)&directionalLight.color);
        ImGui::SliderFloat("Light Multiplier", &dirLightMultiplier, 0.0f, 4.0f);
        
        ImGui::Checkbox("Should show shadows", &shouldShowShadowMap);
        ImGui::Checkbox("Should cull front", &cullFront);
    }
    directionalLight.direction = glm::normalize(directionalLight.direction);
}

void VoxelEngine::createVoxelGrid(std::vector<Model> &objs) {
    voxelSize = 1.0f / gridSize;
    glm::vec4 midpoint = (objs[0].aabb.maxPoint + objs[0].aabb.minPoint) * 0.5f;
    glm::vec4 difference = objs[0].model_matrix * (midpoint - objs[0].aabb.maxPoint);
    maxCoord = glm::compMax(glm::abs(difference)) + 1.0f;
    voxelWorldSize = maxCoord * 2.0f / gridSize;

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

        drawModels(objs, voxelGridPipeline, SKIP_CULLING);
        glGenerateTextureMipmap(voxelGridTexture);
        
        
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
}

std::vector<glm::mat4> VoxelEngine::getLightSpaceMatrices() {
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

glm::mat4 VoxelEngine::getLightSpaceMatrix(float nearPlane, float farPlane) {
    const auto proj = glm::perspective(
        glm::radians(camera->Zoom), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, nearPlane,
        farPlane);
    auto corners = getFrustumCornerWorldSpace(proj, camera->getViewMatrix());

    glm::vec3 center(0, 0, 0);
    for (const auto& v : corners) center += glm::vec3(v);
    center /= corners.size();

    const auto lightView = glm::lookAt(center + directionalLight.direction, center, glm::vec3(0.0f, 1.0f, 0.0f));

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

std::vector<glm::vec4> VoxelEngine::getFrustumCornerWorldSpace(const glm::mat4& proj, const glm::mat4& view) {
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