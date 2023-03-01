#include "gl_compute_eng.h"
#include "utils/gl_funcs.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_stdlib.h"

#include <random>

void ComputeEngine::init_resources() {
    camera = Camera(glm::vec3(0, 0, -1.0f), glm::vec3(0.0, 1.0f, 0.0f), 90.0, 0.0);
    computePipeline = ComputeShader("../../shaders/compute/basic.glsl");

    imgTexture = glutil::createTexture(imgWidth, imgHeight, GL_FLOAT, GL_RGBA, GL_RGBA32F, nullptr);
    glBindImageTexture(0, imgTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

    std::string cubemapPath = "../../resources/textures/skybox/";
    cubemapTexture = glutil::loadCubemap(cubemapPath);

    renderPipeline = Shader("../../shaders/default/defaultScreen.vs", "../../shaders/default/defaultTexture.fs");

    quadBuffer = glutil::createScreenQuad();

    createValues();
}

void ComputeEngine::createValues() {
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<float> color_dist(0.01f, 0.4f);
    std::uniform_real_distribution<float> posz_dist(5.f, 25.f);
    std::uniform_real_distribution<float> pos_dist(-30.f, 30.f);

    for (int i = 0; i < 25; i++) {
        Sphere sphere;
        glm::vec3 color(color_dist(mt), color_dist(mt), color_dist(mt));
        sphere.albedo = (i > 12) ? glm::vec3(0.0f) : color;
        sphere.specular = (i > 12) ? color : glm::vec3(0.5f);
        sphere.origin = glm::vec3(pos_dist(mt), pos_dist(mt), posz_dist(mt));
        sphere.radius = posz_dist(mt);

        spheres.push_back(sphere);
    }

    for (int i = 0; i < 4; i++) {
        PointLight light;
        light.ambient = glm::vec3(color_dist(mt), color_dist(mt), color_dist(mt));
        light.specular = glm::vec3(color_dist(mt), color_dist(mt), color_dist(mt));
        light.diffuse = glm::vec3(color_dist(mt), color_dist(mt), color_dist(mt));
        light.position = glm::vec3(pos_dist(mt), pos_dist(mt), posz_dist(mt));

        pointLights.push_back(light);
    }

    dirLight.ambient = glm::vec3(color_dist(mt), color_dist(mt), color_dist(mt));
    dirLight.specular = glm::vec3(color_dist(mt), color_dist(mt), color_dist(mt));
    dirLight.diffuse = glm::vec3(color_dist(mt), color_dist(mt), color_dist(mt));
    dirLight.direction = glm::vec3(color_dist(mt), color_dist(mt), color_dist(mt));

    plane.normal = glm::vec3(0.0f, 1.0f, 0.0f);
    plane.point = glm::vec3(0.0f, -40.0f, 0.0f);
    plane.albedo = glm::vec3(color_dist(mt), color_dist(mt), color_dist(mt));
    plane.specular = glm::vec3(color_dist(mt), color_dist(mt), color_dist(mt));
}

void ComputeEngine::run() {
    bool closedWindow = false;
    SDL_Event event;
    int shininess = 10;

    while (!closedWindow) {
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        float currentFrame = static_cast<float>(SDL_GetTicks());
        deltaTime = currentFrame - lastFrame;
        deltaTime *= 0.1;
        lastFrame = currentFrame;

        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) {
                closedWindow = true;
            } else if (event.type == SDL_KEYDOWN) {
                SDL_Keycode type = event.key.keysym.sym;

                if (type == SDLK_UP)
                    camera.processKeyboard(FORWARD, deltaTime);
                
                if (type == SDLK_DOWN)
                    camera.processKeyboard(BACKWARD, deltaTime);

                if (type == SDLK_LEFT)
                    camera.processKeyboard(LEFT, deltaTime);

                if (type == SDLK_RIGHT)
                    camera.processKeyboard(RIGHT, deltaTime);
            } else if (event.type == SDL_MOUSEMOTION && !io.WantCaptureMouse) {
                mouse_callback(event.motion.x, event.motion.y);
            } else if (event.type == SDL_MOUSEWHEEL) {
                scroll_callback(event.wheel.y);
            } else if (event.type == SDL_WINDOWEVENT
                && event.window.event == SDL_WINDOWEVENT_RESIZED) {
                framebuffer_callback(event.window.data1, event.window.data2);
            }
        }

        if (importedObjs.size() != 0) {
            Model model = importedObjs[0];
            loadModelData(model);

            importedObjs.pop_back();
            usableObjs.push_back(model);
        }

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)imgWidth/ (float)imgHeight, 0.1f, 100.0f);
        float near = -5.0f, far = 25.0f;
        projection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near, far);
        glm::mat4 inverseView = glm::inverse(projection * camera.getViewMatrix());

        computePipeline.use();
        computePipeline.setFloat("t", currentFrame * 0.01);
        computePipeline.setMat4("inverseView", inverseView);

        for (int i = 0; i < 4; i++) {
            std::string name = "pointLights[" + std::to_string(i) + "]";

            computePipeline.setVec3(name + ".position", pointLights[i].position);
            computePipeline.setVec3(name + ".ambient", pointLights[i].ambient);
            computePipeline.setVec3(name + ".specular", pointLights[i].specular);
            computePipeline.setVec3(name + ".diffuse", pointLights[i].diffuse);
        }

        for (int i = 0; i < 25; i++) {
            std::string name = "spheres[" + std::to_string(i) + "]";

            computePipeline.setVec3(name + ".origin", spheres[i].origin);
            computePipeline.setVec3(name + ".albedo", spheres[i].albedo);
            computePipeline.setVec3(name + ".specular", spheres[i].specular);
            computePipeline.setFloat(name + ".radius", spheres[i].radius);
        }
        std::string name = "plane";
        computePipeline.setVec3(name + ".point", plane.point);
        computePipeline.setVec3(name + ".normal", plane.normal);
        computePipeline.setVec3(name + ".albedo", plane.albedo);
        computePipeline.setVec3(name + ".specular", plane.specular);

        name = "dirLight";
        computePipeline.setVec3(name + ".direction", dirLight.direction);
        computePipeline.setVec3(name + ".diffuse", dirLight.diffuse);
        computePipeline.setVec3(name + ".ambient", dirLight.ambient);
        computePipeline.setVec3(name + ".specular", dirLight.specular);

        computePipeline.setInt("numReflections", numReflections);
        computePipeline.setInt("shininess", shininess);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        computePipeline.setInt("cubemap", 1);

        glDispatchCompute((unsigned int) imgWidth/8, (unsigned int) imgHeight/4, 1);

        glClearColor(1.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Info");
        if (ImGui::CollapsingHeader("Spheres")) {
            for (int i = 0; i < 15; i++) {
                std::string name = "Sphere  " + std::to_string(i);
                auto& currentLight = spheres[i];
                if (ImGui::TreeNode(name.c_str())) {
                    ImGui::SliderFloat3("Position", (float*)&currentLight.origin, -50.0, 50.0);
                    ImGui::SliderFloat("Radius",(float*) &currentLight.radius, 1.0, 100.0);
                    ImGui::SliderFloat3("Albedo", (float*)&currentLight.albedo, 0.0, 1.0);
                    ImGui::SliderFloat3("Specular", (float*)&currentLight.specular, 0.0, 1.0);
                    ImGui::TreePop();
                }
            }
        }
        if (ImGui::CollapsingHeader("Lights")) {
            for (int i = 0; i < 4; i++) {
                std::string name = "Light  " + std::to_string(i);
                auto& currentLight = pointLights[i];
                if (ImGui::TreeNode(name.c_str())) {
                    ImGui::SliderFloat3("Position", (float*)&currentLight.position, -50.0, 50.0);
                    ImGui::SliderFloat3("Ambient",(float*) &currentLight.ambient, 0.0, 1.0);
                    ImGui::SliderFloat3("Diffuse ", (float*)&currentLight.diffuse, 0.0, 1.0);
                    ImGui::SliderFloat3("Specular", (float*)&currentLight.specular, 0.0, 1.0);
                    ImGui::TreePop();
                }
            }
        }
        if (ImGui::CollapsingHeader("Directional Light")) {
            ImGui::SliderFloat3("Direction", (float*)&dirLight.direction, -1.0, 1.0);
            ImGui::SliderFloat3("Ambient",(float*) &dirLight.ambient, 0.0, 1.0);
            ImGui::SliderFloat3("Diffuse ", (float*)&dirLight.diffuse, 0.0, 1.0);
            ImGui::SliderFloat3("Specular", (float*)&dirLight.specular, 0.0, 1.0);
        }
        if (ImGui::CollapsingHeader("Plane Info")) {
            ImGui::SliderFloat3("Point", (float*)&plane.point, -50.0, 50.0);
            ImGui::SliderFloat3("Normal", (float*)&plane.normal, -1.0, 1.0);
            ImGui::SliderFloat3("Albedo", (float*)&plane.albedo, 0.0, 1.0);
            ImGui::SliderFloat3("Specular", (float*)&plane.specular, 0.0, 1.0);
        }
        if (ImGui::CollapsingHeader("Camera Info")) {
            ImGui::SliderFloat3("Front", (float*)&camera.Front, -1.0, 1.0);
            ImGui::SliderFloat3("Position", (float*)&camera.Position, -100.0, 100.0);
            ImGui::SliderFloat3("Up", (float*)&camera.Up, -100.0, 100.0);

            ImGui::SliderInt("Num of Reflections", (int*)&numReflections, 1, 10);
            ImGui::SliderInt("Shininess", (int*)&shininess, 5, 100);
        }
        ImGui::End();

        renderPipeline.use();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, imgTexture);
        renderPipeline.setInt("texture1", 0);
        glBindVertexArray(quadBuffer.VAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }
}