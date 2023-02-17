#include "gl_engine.h"
#include "gl_funcs.h"

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

void RenderEngine::init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        printf("Error: %s \n", SDL_GetError());
        return;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
        | SDL_WINDOW_ALLOW_HIGHDPI);
    window = SDL_CreateWindow("GL Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, window_flags);
    if (window == nullptr) {
        printf("Error: %s \n", SDL_GetError());
        return;
    }

    gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    gladLoadGLLoader(SDL_GL_GetProcAddress);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 460");

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);

    init_resources();
}

void RenderEngine::init_resources() {
    camera = Camera(glm::vec3(0.0f, 0.0f, 7.0f));
    
    pipeline = Shader("../../shaders/animation/model.vs", "../../shaders/shadowPoints/model.fs");
    mapPipeline = Shader("../../shaders/cubemap/map.vs", "../../shaders/cubemap/map.fs");
    shadowmapPipeline = Shader("../../shaders/shadows/map.vs", "../../shaders/shadows/map.fs");
    depthCubemapPipeline = Shader("../../shaders/shadowPoints/map.vs", "../../shaders/shadowPoints/map.fs",
        "../../shaders/shadowPoints/map.gs");
    
    Model newModel("../../resources/objects/raptoid/scene.gltf");
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

    float planeVertices[] = {
        // positions            // normals         // texcoords
         25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  1.0f,  0.0f,
        -25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
        -25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

         25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  1.0f,  0.0f,
        -25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
         25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f
    };
    std::vector<float> newPlaneVertices(std::begin(planeVertices), std::end(planeVertices));
    planeBuffer = glutil::loadVertexBuffer(newPlaneVertices);
    planeTexture = glutil::loadTexture("../../resources/textures/wood.png");

    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f,  1.0f
    };
    std::vector<float> newSkyboxVertices(std::begin(skyboxVertices), std::end(skyboxVertices));
    std::vector<VertexType> types = {POSITION};
    cubemapBuffer = glutil::loadVertexBuffer(newSkyboxVertices, types);

    float quadVertices[] = {
        // positions        // texture Coords
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };
    types = {POSITION, TEXCOORDS};
    std::vector<float> newQuadVertices(std::begin(quadVertices), std::end(quadVertices));
    quadBuffer = glutil::loadVertexBuffer(newQuadVertices, types);

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

    computePipeline = ComputeShader("../../shaders/compute/tutorial.glsl");

    imgTexture = glutil::createTexture(imgWidth, imgHeight, GL_FLOAT, GL_RGBA, GL_RGBA32F, nullptr);
    glBindImageTexture(0, imgTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
}

void RenderEngine::run() {
    bool closedWindow = false;
    SDL_Event event;
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
            } else if (event.type == SDL_MOUSEMOTION && (!io.WantCaptureMouse || ImGuizmo::IsOver())) {
                mouse_callback(event.motion.x, event.motion.y);
            } else if (event.type == SDL_MOUSEWHEEL) {
                scroll_callback(event.wheel.y);
            } else if (event.type == SDL_WINDOWEVENT
                && event.window.event == SDL_WINDOWEVENT_RESIZED) {
                framebuffer_callback(event.window.data1, event.window.data2);
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                handleClick(event.motion.x, event.motion.y);
            }
        }
        glm::mat4 inverseView = glm::inverse(camera.getViewMatrix());
        computePipeline.use();
        computePipeline.setFloat("t", currentFrame * 0.01);
        computePipeline.setMat4("inverseView", inverseView);
        glDispatchCompute((unsigned int) imgWidth/8, (unsigned int) imgHeight/4, 1);

        glClearColor(1.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

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
                    || ImGui::SliderFloat3("Scale", (float*)&scale, 1.0, 10.0)) {
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

        if (importedObjs.size() != 0) {
            Model model = importedObjs[0];
            loadModelData(model);

            importedObjs.pop_back();
            usableObjs.push_back(model);
        }

        pipeline.use();

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)WINDOW_WIDTH/ (float)WINDOW_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.getViewMatrix();

        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
        ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), operation,
            ImGuizmo::LOCAL, glm::value_ptr(usableObjs[chosenObjIndex].model_matrix));

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

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        pipeline.setInt("shadowMap", 3);

        for (int i = 0; i < 4; i++) {
            glActiveTexture(GL_TEXTURE4 + i);
            glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemaps[i]);
            pipeline.setInt("shadowMaps[" + std::to_string(i) + "]", 4 + i);
        }

        drawModels(pipeline);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        pipeline.setMat4("model", planeModel);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, imgTexture);
        pipeline.setInt("diffuseTexture", 0);
        pipeline.setInt("shadowMap", 3);
        glBindVertexArray(planeBuffer.VAO);

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

void RenderEngine::drawModels(Shader& shader, bool skipTextures) {
    for (Model& model : usableObjs) {
        shader.setMat4("model", model.model_matrix);

        for (int j = 0; j < model.meshes.size(); j++) {
            Mesh& mesh = model.meshes[j];
            if (!skipTextures) {
                unsigned int diffuseNr = 1;
                unsigned int specularNr = 1;
                unsigned int normalNr = 1;
                unsigned int heightNr = 1;

                for (unsigned int i = 0; i < mesh.textures.size(); i++) {
                    glActiveTexture(GL_TEXTURE0 + i);

                    string number;
                    string name = mesh.textures[i].type;
                    
                    if (name == "texture_diffuse")
                        number = std::to_string(diffuseNr++);
                    else if (name == "texture_specular")
                        number = std::to_string(specularNr++);
                    else if (name == "texture_normal")
                        number = std::to_string(normalNr++);
                    else if (name == "texture_height")
                        number = std::to_string(heightNr++);

                    string key = name + number;
                    shader.setInt(key.c_str(), i);

                    glBindTexture(GL_TEXTURE_2D, mesh.textures[i].id);
                }
                glActiveTexture(GL_TEXTURE0);

                if (mesh.bone_data.size() != 0) {
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, mesh.SSBO);

                    mesh.getBoneTransforms(animationTime, model.scene, model.nodes, chosenAnimation);
                    std::string boneString = "boneMatrices[";
                    for (unsigned int i = 0; i < mesh.bone_info.size(); i++) {
                        shader.setMat4(boneString + std::to_string(i) + "]", 
                            mesh.bone_info[i].finalTransform);
                    }
                }
            }

            glBindVertexArray(mesh.buffer.VAO);
            glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
    }
}

void RenderEngine::cleanup() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void RenderEngine::async_load_model(std::string path) {
    Model newModel(path);

    importedObjs.push_back(newModel);
}

void RenderEngine::loadModelData(Model& model) {
    for (Texture& texture : model.textures_loaded) {
        unsigned int textureID;
        glCreateTextures(GL_TEXTURE_2D, 1, &textureID);
        texture.id = textureID;

        GLenum format;
        GLenum storageFormat;
        if (texture.nrComponents == 1) {
            format = GL_RED;
            storageFormat = GL_R8;
        } else if (texture.nrComponents == 3) {
            format = GL_RGB;
            storageFormat = GL_RGB8;
        } else if (texture.nrComponents == 4) {
            format = GL_RGBA;
            storageFormat = GL_RGBA8;
        }

        glTextureStorage2D(textureID, 1, storageFormat, texture.width, texture.height);
        glTextureSubImage2D(textureID, 0, 0, 0, texture.width, texture.height, format, GL_UNSIGNED_BYTE, texture.data);
        glGenerateTextureMipmap(textureID);

        glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(texture.data);
    }

    for (Mesh& mesh : model.meshes) {
        std::vector<VertexType> endpoints = {POSITION, NORMAL, TEXCOORDS, TANGENT, BI_TANGENT, VERTEX_ID};
        mesh.buffer = glutil::loadVertexBuffer(mesh.vertices, mesh.indices, endpoints);
        
        if (mesh.bone_data.size() != 0) {
            glCreateBuffers(1, &mesh.SSBO);
            glNamedBufferStorage(mesh.SSBO, sizeof(VertexBoneData) * mesh.bone_data.size(),
                mesh.bone_data.data(), GL_DYNAMIC_STORAGE_BIT);
        }

        for (std::string& path : mesh.texture_paths) {
            for (unsigned int j = 0; j < model.textures_loaded.size(); j++) {
                if (std::strcmp(model.textures_loaded[j].path.data(), path.c_str()) == 0) {
                    mesh.textures.push_back(model.textures_loaded[j]);
                    break;
                }
            }
        }
    }
}

void RenderEngine::framebuffer_callback(int width, int height) {
    glViewport(0, 0, width, height);
    WINDOW_HEIGHT = height;
    WINDOW_WIDTH = width;
}

void RenderEngine::mouse_callback(double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    camera.processMouseMovement(xoffset, yoffset);
}

void RenderEngine::scroll_callback(double yoffset) {
    camera.processMouseScroll(static_cast<float>(yoffset));
}

void RenderEngine::handleClick(double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)WINDOW_WIDTH/ (float)WINDOW_HEIGHT, 0.1f, 100.0f);

    float convertedX = (2.0f * xpos) / WINDOW_WIDTH - 1.0f;
    float convertedY = (2.0f * ypos) / WINDOW_HEIGHT - 1.0f;
    convertedY *= -1;
    glm::vec4 ray_clip(convertedX, convertedY, -1.0f, 1.0f);
    glm::vec4 ray_eye = glm::inverse(projection) * ray_clip;
    ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0f, 1.0f);
    glm::vec4 ray_world = glm::inverse(camera.getViewMatrix()) * ray_eye;

    glm::vec4 ray_origin = glm::vec4(camera.Position.x, camera.Position.y, camera.Position.z, 1.0f);
    glm::vec4 ray_dir = glm::normalize(ray_world - ray_origin);
    glm::vec4 inverse_dir = glm::vec4(1.0f) / ray_dir;

    checkIntersection(ray_origin, ray_dir, inverse_dir);
}

void RenderEngine::checkIntersection(glm::vec4& origin, glm::vec4& direction, glm::vec4& inverse_dir) {
    for (int i = 0; i < usableObjs.size(); i++) {
        glm::vec4 boxMin = usableObjs[i].model_matrix * usableObjs[i].aabb.minPoint;
        glm::vec4 boxMax = usableObjs[i].model_matrix * usableObjs[i].aabb.maxPoint;

        float tmin = -INFINITY, tmax = INFINITY;
        if (direction.x != 0.0f) {
            float t1 = (boxMin.x - origin.x) * inverse_dir.x;
            float t2 = (boxMax.x - origin.x) * inverse_dir.x;

            tmin = std::max(tmin, std::min(t1, t2));
            tmax = std::min(tmax, std::max(t1, t2));
        }

        if (direction.y != 0.0f) {
            float t1 = (boxMin.y - origin.y) * inverse_dir.y;
            float t2 = (boxMax.y - origin.y) * inverse_dir.y;

            tmin = std::max(tmin, std::min(t1, t2));
            tmax = std::min(tmax, std::max(t1, t2));
        }

        if (direction.z != 0.0f) {
            float t1 = (boxMin.z - origin.z) * inverse_dir.z;
            float t2 = (boxMax.z - origin.z) * inverse_dir.z;

            tmin = std::max(tmin, std::min(t1, t2));
            tmax = std::min(tmax, std::max(t1, t2));
        }

        if (tmax >= tmin) {
            chosenObjIndex = i;
            break;
        }
    }
}