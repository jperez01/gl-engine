#include "gl_engine.h"
#include "gl_funcs.h"

#include <iostream>
#include <iterator>
#include <SDL.h>
#include <thread>
#include <future>
#include <glm/gtc/matrix_transform.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_stdlib.h"

void GLEngine::init() {
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

void GLEngine::init_resources() {
    camera = Camera(glm::vec3(0.0f, 0.0f, 7.0f));
    
    pipeline = Shader("../../shaders/model.vs", "../../shaders/model.fs");
    mapPipeline = Shader("../../shaders/cubemap/map.vs", "../../shaders/cubemap/map.fs");
    shadowmapPipeline = Shader("../../shaders/shadows/map.vs", "../../shaders/shadows/map.fs");
    
    Model newModel("../../resources/objects/backpack/backpack.obj");
    loadModelData(newModel);
    usableObjs.push_back(newModel);

    glCreateFramebuffers(1, &depthFBO);
    glCreateTextures(GL_TEXTURE_2D, 1, &depthMap);
    glTextureStorage2D(depthMap, 1, GL_DEPTH_COMPONENT32F, 2048, 2048);
    glNamedFramebufferTexture(depthFBO, GL_DEPTH_ATTACHMENT, depthMap, 0);
    glNamedFramebufferDrawBuffer(depthFBO, GL_NONE);
    glNamedFramebufferReadBuffer(depthFBO, GL_NONE);

    float planeVertices[] = {
        // positions            // normals         // texcoords
         25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
        -25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
        -25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,

         25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
        -25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,
         25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,  25.0f, 25.0f
    };
    std::vector<float> newPlaneVertices(std::begin(planeVertices), std::end(planeVertices));
    planeBuffer = glutil::loadOldSimpleVertexBuffer(newPlaneVertices, glutil::WITH_TEXCOORDS);
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
    cubemapBuffer = glutil::loadOldSimpleVertexBuffer(newSkyboxVertices);

    float quadVertices[] = {
        // positions        // texture Coords
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };
    std::vector<float> newQuadVertices(std::begin(quadVertices), std::end(quadVertices));
    quadBuffer = glutil::loadOldSimpleVertexBuffer(newQuadVertices, glutil::WITH_TEXCOORDS);

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
}

void GLEngine::run() {
    bool closedWindow = false;
    SDL_Event event;
    float shininess = 100.0;
    std::string path = "";
    glm::vec3 translate = glm::vec3(0.0), rotation = glm::vec3(0.0), scale = glm::vec3(1.0);
    int chosenObjIndex;

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
        glClearColor(1.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glm::mat4 lightProjection, lightView, lightSpaceMatrix;
        float near = 1.0f, far = 7.5f;
        glm::vec3 someLightPos(-2.0f, 4.0f, -1.0f);
        lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near, far);
        lightView = glm::lookAt(pointLights[0].position, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
        lightSpaceMatrix = lightProjection * lightView;

        shadowmapPipeline.use();
        shadowmapPipeline.setMat4("lightSpaceMatrix", lightSpaceMatrix);

        glViewport(0, 0, 2048, 2048);
        glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        drawModels(shadowmapPipeline, true);

        glm::mat4 planeModel = glm::mat4(1.0f);
        planeModel = glm::translate(planeModel, glm::vec3(0.0, -2.0, 0.0));
        shadowmapPipeline.setMat4("model", planeModel);
        glBindVertexArray(planeBuffer.VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

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
                std::thread(&GLEngine::async_load_model, this, chosenPath).detach();
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

        pipeline.setMat4("projection", projection);
        pipeline.setMat4("view", view);
        pipeline.setFloat("shininess", shininess);

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
        pipeline.setMat4("lightSpaceMatrix", lightSpaceMatrix);

        drawModels(pipeline);

        pipeline.setMat4("model", planeModel);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, planeTexture);
        pipeline.setInt("diffuseTexture", 0);
        pipeline.setInt("shadowMap", 3);
        glBindVertexArray(planeBuffer.VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

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

void GLEngine::drawModels(Shader& shader, bool skipTextures) {
    for (Model& model : usableObjs) {
        shader.setMat4("model", model.model_matrix);

        for (Mesh& mesh : model.meshes) {
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
            }

            glBindVertexArray(mesh.VAO);
            glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
    }
}

void GLEngine::cleanup() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void GLEngine::async_load_model(std::string path) {
    Model newModel(path);

    importedObjs.push_back(newModel);
}

void GLEngine::loadModelData(Model& model) {
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
        GLuint binding_index = 0;

        glCreateVertexArrays(1, &mesh.VAO);
        glCreateBuffers(1, &mesh.EBO);
        glCreateBuffers(1, &mesh.VBO);

        glNamedBufferStorage(mesh.VBO, sizeof(Vertex) * mesh.vertices.size(), mesh.vertices.data(), GL_DYNAMIC_STORAGE_BIT);
        glNamedBufferStorage(mesh.EBO, mesh.indices.size() * sizeof(unsigned int), mesh.indices.data(), GL_DYNAMIC_STORAGE_BIT);
        
        glBindVertexArray(mesh.VAO);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
        glVertexArrayElementBuffer(mesh.VAO, mesh.EBO);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, Normal));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, TexCoords));
        // vertex tangent
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
        // vertex bitangent
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));

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

void framebuffer_callback(int width, int height) {
    glViewport(0, 0, width, height);
}

void GLEngine::mouse_callback(double xposIn, double yposIn) {
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

void GLEngine::scroll_callback(double yoffset) {
    camera.processMouseScroll(static_cast<float>(yoffset));
}