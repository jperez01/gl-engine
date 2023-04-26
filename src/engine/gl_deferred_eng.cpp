#include "gl_deferred_eng.h"

#include "glm/gtx/string_cast.hpp"
#include <random>

void DeferredEngine::init_resources() {
    renderPipeline = Shader("deferred/lighting.vs", "ssr/finalPassF.glsl");
    gbufferPipeline = Shader("aliasing/taa/taaGbuffer.vs", "aliasing/taa/taaGbuffer.fs");
    fxaaPipeline = Shader("deferred/lighting.vs", "aliasing/fxaa.fs");
    ssrPipeline = Shader("deferred/lighting.vs", "ssr/ssrF.glsl");

    taaResolvePipeline = Shader("deferred/lighting.vs", "aliasing/taa/taaResolve.fs");
    taaHistoryPipeline = Shader("deferred/lighting.vs", "aliasing/taa/taaHistory.fs");

    inverseScreenSize = glm::vec2(1.0 / WINDOW_WIDTH, 1.0 / WINDOW_HEIGHT);
   for (int i = 0; i < 128; i++) {
    haltonSequences[i] = glm::vec2(createHaltonSequence(i + 1, 2), createHaltonSequence(i + 1, 3));
   }

    quadBuffer = glutil::createScreenQuad();

    planeBuffer = glutil::createPlane();
    planeTexture = glutil::loadTexture("../../resources/textures/wood.png");
    planeModel = glm::mat4(1.0f);
    planeModel = glm::translate(planeModel, glm::vec3(0.0, 0.0, 0.0));
    planeModel = glm::scale(planeModel, glm::vec3(1.0f, 1.5f, 1.0f));

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
    directionalLight.direction = glm::vec3(0.0f, 1.0f, 0.0f);
    directionalLight.color = glm::vec3(1.0f, 1.0f, 1.0f);

    glCreateFramebuffers(1, &deferredFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, deferredFBO);

    gPosition = glutil::createTexture(WINDOW_WIDTH, WINDOW_HEIGHT, GL_FLOAT, GL_RGBA, GL_RGBA16F, nullptr);
    gNormal = glutil::createTexture(WINDOW_WIDTH, WINDOW_HEIGHT, GL_FLOAT, GL_RGBA, GL_RGBA16F, nullptr);
    gAlbedo = glutil::createTexture(WINDOW_WIDTH, WINDOW_HEIGHT, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA8, nullptr);
    gReflectionColor = glutil::createTexture(WINDOW_WIDTH, WINDOW_HEIGHT, GL_FLOAT, GL_RGBA, GL_RGBA16F);
    gReflectionPosition = glutil::createTexture(WINDOW_WIDTH, WINDOW_HEIGHT, GL_FLOAT, GL_RGBA, GL_RGBA16F);
    gMetallic = glutil::createTexture(WINDOW_WIDTH, WINDOW_HEIGHT, GL_FLOAT, GL_RG, GL_RG16F, nullptr);
    gVelocity = glutil::createTexture(WINDOW_WIDTH, WINDOW_HEIGHT, GL_FLOAT, GL_RG, GL_RG16F, nullptr);

    depthMap = glutil::createTexture(WINDOW_WIDTH, WINDOW_HEIGHT, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, GL_DEPTH_STENCIL, GL_DEPTH32F_STENCIL8);
    
    glNamedFramebufferTexture(deferredFBO, GL_COLOR_ATTACHMENT0, gPosition, 0);
    glNamedFramebufferTexture(deferredFBO, GL_COLOR_ATTACHMENT1, gNormal, 0);
    glNamedFramebufferTexture(deferredFBO, GL_COLOR_ATTACHMENT2, gAlbedo, 0);
    glNamedFramebufferTexture(deferredFBO, GL_COLOR_ATTACHMENT3, gReflectionPosition, 0);
    glNamedFramebufferTexture(deferredFBO, GL_COLOR_ATTACHMENT4, gMetallic, 0);
    glNamedFramebufferTexture(deferredFBO, GL_COLOR_ATTACHMENT5, gVelocity, 0);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthMap, 0);

    glNamedFramebufferTexture(deferredFBO, GL_DEPTH_STENCIL_ATTACHMENT, depthMap, 0);

    unsigned int attachments[6] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, 
        GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5 };
    glNamedFramebufferDrawBuffers(deferredFBO, 6, attachments);
    
    if (glCheckNamedFramebufferStatus(deferredFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "Framebuffer not complete! " << std::endl;
    }
    glBindFramebuffer(deferredFBO, 0);

    glCreateFramebuffers(1, &ssrFBO);
    glNamedFramebufferTexture(ssrFBO, GL_COLOR_ATTACHMENT0, gReflectionColor, 0);
    glNamedFramebufferDrawBuffer(ssrFBO, GL_COLOR_ATTACHMENT0);

    unsigned int ssrDepth = glutil::createTexture(WINDOW_WIDTH, WINDOW_HEIGHT, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, GL_DEPTH_STENCIL, GL_DEPTH32F_STENCIL8);
    glNamedFramebufferTexture(ssrFBO, GL_DEPTH_STENCIL_ATTACHMENT, ssrDepth, 0);

    if (glCheckNamedFramebufferStatus(ssrFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "Framebuffer not complete! " << std::endl;
    }

    glCreateFramebuffers(1, &aaFBO);
    colorTexture = glutil::createTexture(WINDOW_WIDTH, WINDOW_HEIGHT, GL_FLOAT, GL_RGBA, GL_RGBA8, nullptr);
    glNamedFramebufferTexture(aaFBO, GL_COLOR_ATTACHMENT0, colorTexture, 0);
    glNamedFramebufferDrawBuffer(aaFBO, GL_COLOR_ATTACHMENT0);

    if (glCheckNamedFramebufferStatus(aaFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "Framebuffer not complete! " << std::endl;
    }

    glCreateFramebuffers(1, &simpleColorFBO);
    simpleColorTexture = glutil::createTexture(WINDOW_WIDTH, WINDOW_HEIGHT, GL_FLOAT, GL_RGBA, GL_RGBA8, nullptr);
    glNamedFramebufferTexture(simpleColorFBO, GL_COLOR_ATTACHMENT0, simpleColorTexture, 0);
    glNamedFramebufferDrawBuffer(simpleColorFBO, GL_COLOR_ATTACHMENT0);

    if (glCheckNamedFramebufferStatus(simpleColorFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "Framebuffer not complete! " << std::endl;
    }

    glCreateFramebuffers(1, &historyFBO);
    historyColorTexture = glutil::createTexture(WINDOW_WIDTH, WINDOW_HEIGHT, GL_FLOAT, GL_RGBA, GL_RGBA8, nullptr);
    glNamedFramebufferTexture(historyFBO, GL_COLOR_ATTACHMENT0, historyColorTexture, 0);
    glNamedFramebufferDrawBuffer(historyFBO, GL_COLOR_ATTACHMENT0);

    if (glCheckNamedFramebufferStatus(historyFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "Framebuffer not complete! " << std::endl;
    }
}


void DeferredEngine::render(std::vector<Model>& objs) {
    jitterIndex = jitterIndex % 128;
    jitter = haltonSequences[jitterIndex] * inverseScreenSize;
    jitterIndex++;

    glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom), (float)WINDOW_WIDTH/ (float)WINDOW_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera->getViewMatrix();
    glm::mat4 model = glm::mat4(1.0f);

    glBindFramebuffer(GL_FRAMEBUFFER, deferredFBO);
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        gbufferPipeline.use();
        gbufferPipeline.setMat4("projection", projection);
        gbufferPipeline.setMat4("view", view);

        gbufferPipeline.setMat4("prevProjection", prevProjection);
        gbufferPipeline.setMat4("prevView", prevView);
        if (shouldFXAA)
            gbufferPipeline.setVec2("jitter", glm::vec2(0.0f));
        else 
            gbufferPipeline.setVec2("jitter", jitter);

        glBindTextureUnit(0, planeTexture);
        gbufferPipeline.setInt("texture_diffuse1", 0);

        gbufferPipeline.setMat4("model", planeModel);

        glBindVertexArray(planeBuffer.VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(0.1f));
        objs[0].model_matrix = model;

        gbufferPipeline.setMat4("model", model);
        drawModels(objs, gbufferPipeline);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, ssrFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ssrPipeline.use();
        glBindTextureUnit(0, depthMap);
        glBindTextureUnit(1, gNormal);
        glBindTextureUnit(2, gAlbedo);
        glBindTextureUnit(3, gReflectionPosition);
        glBindTextureUnit(4, gMetallic);

        ssrPipeline.setInt("normalTexture", 1);
        ssrPipeline.setInt("colorTexture", 2);
        ssrPipeline.setInt("depthTexture", 0);
        ssrPipeline.setInt("positionTexture", 3);
        ssrPipeline.setInt("metallicRoughnessTexture", 4);

        ssrPipeline.setMat4("projection", projection);
        ssrPipeline.setMat4("view", view);
        ssrPipeline.setMat4("invProjection", glm::inverse(projection));
        ssrPipeline.setMat4("invView", glm::inverse(view));
        glBindVertexArray(quadBuffer.VAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (shouldFXAA) 
        glBindFramebuffer(GL_FRAMEBUFFER, aaFBO);
    else 
        glBindFramebuffer(GL_FRAMEBUFFER, simpleColorFBO);
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

        renderPipeline.setVec3("directionalLight.direction", directionalLight.direction);
        renderPipeline.setVec3("directionalLight.color", directionalLight.color);

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
            renderPipeline.setFloat(name + "Radius", globalRadius);
        }
        renderPipeline.setVec3("viewPos", camera->Position);
            
        glBindVertexArray(quadBuffer.VAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (shouldFXAA) {
        renderFXAA();
    } else {
        renderTAA();
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, deferredFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    prevProjection = projection;
    prevView = view;
}

void DeferredEngine::handleImGui() {
    ImGuiIO& io = ImGui::GetIO(); (void)io;

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
        ImGui::SliderFloat3("Light Direction", (float*)&directionalLight.direction, -1.0f, 1.0f);
        ImGui::SliderFloat3("Dir Light Color", (float*)&directionalLight.color, 0.0f, 1.0f);

        ImGui::SliderFloat("Camera Multiplier", &multiplier, 0.00001f, 0.01f);
        ImGui::SliderFloat("Global Radius ", &globalRadius, 1.0f, 10000.0f);
        ImGui::SliderFloat("Step Multiplier", &stepMultiplier, 0.5f, 10.0f);

        ImGui::Checkbox("Use FXAA", &shouldFXAA);

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

    glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom), (float)WINDOW_WIDTH/ (float)WINDOW_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera->getViewMatrix();

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), operation,
        ImGuizmo::LOCAL, glm::value_ptr(planeModel));
}

float DeferredEngine::createHaltonSequence(unsigned int index, int base) {
    float f = 1;
    float r = 0;
    int current = index;
    do
    {
        f = f / base;
        r = r + f * (current % base);
        current = glm::floor(current / base);
    } while (current > 0);
    return r;
}

void DeferredEngine::renderFXAA() {
    fxaaPipeline.use();
    glBindTextureUnit(0, colorTexture);

    fxaaPipeline.setVec2("inverseScreenSize", inverseScreenSize);
    fxaaPipeline.setInt("screenTexture", 0);
    fxaaPipeline.setFloat("multiplier", stepMultiplier);
    glBindVertexArray(quadBuffer.VAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void DeferredEngine::renderTAA() {
    glBindFramebuffer(GL_FRAMEBUFFER, aaFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        taaResolvePipeline.use();
        glBindTextureUnit(0, simpleColorTexture);
        glBindTextureUnit(1, historyColorTexture);
        glBindTextureUnit(2, gVelocity);

        taaResolvePipeline.setInt("currentColorBuffer", 0);
        taaResolvePipeline.setInt("historyBuffer", 1);
        taaResolvePipeline.setInt("velocityBuffer", 2);
        glBindVertexArray(quadBuffer.VAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, historyFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        taaHistoryPipeline.use();
        glBindTextureUnit(0, colorTexture);

        taaHistoryPipeline.setInt("colorTexture", 0);
        glBindVertexArray(quadBuffer.VAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    taaHistoryPipeline.use();
    glBindTextureUnit(0, colorTexture);

    taaHistoryPipeline.setInt("colorTexture", 0);
    glBindVertexArray(quadBuffer.VAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}