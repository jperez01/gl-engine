#include "gl_pbr_engine.h"
#include "utils/gl_funcs.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_stdlib.h"
#include "ImGuizmo.h"

void PBREngine::init_resources() {
    camera = Camera(glm::vec3(0.0f, 0.0f, 10.0f));

    pipeline = Shader("../../shaders/pbr/basicVertex.glsl", "../../shaders/pbr/basicFragment.glsl");
    convertToCubemapPipeline = Shader("../../shaders/pbr/cubemapVertex.glsl", "../../shaders/pbr/cubemapFragment.glsl");
    createIrradiancePipeline = Shader("../../shaders/pbr/cubemapVertex.glsl", "../../shaders/pbr/irradianceFragment.glsl");
    backgroundPipeline = Shader("../../shaders/pbr/backgroundV.glsl", "../../shaders/pbr/backgroundF.glsl");
    prefilterPipeline = Shader("../../shaders/pbr/cubemapVertex.glsl", "../../shaders/pbr/prefilterF.glsl");
    brdfPipeline = Shader("../../shaders/pbr/brdfV.glsl", "../../shaders/pbr/brdfF.glsl");

    Model newModel("../../resources/objects/DamagedHelmet/DamagedHelmet.gltf", GLTF);
    loadModelData(newModel);
    usableObjs.push_back(newModel);

    albedoMap = glutil::loadTexture("../../resources/textures/reinforced-metal/reinforced-metal_albedo.png");
    aoMap = glutil::loadTexture("../../resources/textures/reinforced-metal/reinforced-metal_ao.png");
    normalMap = glutil::loadTexture("../../resources/textures/reinforced-metal/reinforced-metal_normal-dx.png");
    metallicMap = glutil::loadTexture("../../resources/textures/reinforced-metal/reinforced-metal_metallic.png");
    roughnessMap = glutil::loadTexture("../../resources/textures/reinforced-metal/reinforced-metal_roughness.png");

    createSphere();

    lightPositions = {
        glm::vec3(-10.0f,  10.0f, 10.0f),
        glm::vec3( 10.0f,  10.0f, 10.0f),
        glm::vec3(-10.0f, -10.0f, 10.0f),
        glm::vec3( 10.0f, -10.0f, 10.0f),
    };
    lightColors = {
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)
    };

    hdrTexture = glutil::loadFloatTexture("../../resources/textures/apartment/apartment.hdr", GL_RGB, GL_RGB16F);
    brdfLUTTexture = glutil::createTexture(512, 512, GL_FLOAT, GL_RG, GL_RG16F);

    cubemapBuffer = glutil::createUnitCube();
    quadBuffer = glutil::createScreenQuad();
    cubemapTexture = glutil::createCubemap(512, 512, GL_FLOAT, GL_RGB, GL_RGB16F);
    irradianceMap = glutil::createCubemap(32, 32, GL_FLOAT, GL_RGB, GL_RGB16F);

    prefilterMap = glutil::createCubemap(128, 128, GL_FLOAT, GL_RGB, GL_RGB16F);
    glTextureParameteri(prefilterMap, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateTextureMipmap(prefilterMap);

    glCreateFramebuffers(1, &captureFBO);
    glCreateRenderbuffers(1, &captureRBO);
    glCreateRenderbuffers(1, &irradianceRBO);

    glNamedRenderbufferStorage(captureRBO, GL_DEPTH_COMPONENT24, 512, 512);
    glNamedFramebufferRenderbuffer(captureFBO, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

    glNamedRenderbufferStorage(irradianceRBO, GL_DEPTH_ATTACHMENT, 32, 32);
}

void PBREngine::createSphere() {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> uv;
    std::vector<glm::vec3> normals;
    std::vector<unsigned int> indices;

    const unsigned int X_SEGMENTS = 64;
    const unsigned int Y_SEGMENTS = 64;
    const float PI = 3.14159265359f;

    for (unsigned int x = 0; x <= X_SEGMENTS; x++) {
        for (unsigned int y = 0; y <= Y_SEGMENTS; y++) {
            float xSegment = (float)x / (float)X_SEGMENTS;
            float ySegment = (float)y / (float)Y_SEGMENTS;

            float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
            float yPos = std::cos(ySegment * PI);
            float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

            positions.push_back(glm::vec3(xPos, yPos, zPos));
            uv.push_back(glm::vec2(xSegment, ySegment));
            normals.push_back(glm::vec3(xPos, yPos, zPos));
        }
    }

    bool oddRow = false;
    for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
    {
        if (!oddRow) // even rows: y == 0, y == 2; and so on
        {
            for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
            {
                indices.push_back(y       * (X_SEGMENTS + 1) + x);
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
            }
        }
        else
        {
            for (int x = X_SEGMENTS; x >= 0; --x)
            {
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                indices.push_back(y       * (X_SEGMENTS + 1) + x);
            }
        }
        oddRow = !oddRow;
    }
    indexCount = static_cast<unsigned int>(indices.size());

    std::vector<float> data;
    for (unsigned int i = 0; i < positions.size(); i++) {
        data.push_back(positions[i].x);
        data.push_back(positions[i].y);
        data.push_back(positions[i].z);

        if (normals.size() > 0) {
            data.push_back(normals[i].x);
            data.push_back(normals[i].y);
            data.push_back(normals[i].z);
        }

        if (uv.size() > 0) {
            data.push_back(uv[i].x);
            data.push_back(uv[i].y);
        }
    }

    std::vector<VertexType> endpoints = {POSITION, NORMAL, TEXCOORDS};
    sphereBuffer = glutil::loadVertexBuffer(data, indices, endpoints);
}

void PBREngine::createIrradianceMap() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
    glm::mat4 captureViews[] = 
        {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
        };
    
    convertToCubemapPipeline.use();
    convertToCubemapPipeline.setMat4("projection", captureProjection);
    glBindTextureUnit(0, hdrTexture);
    convertToCubemapPipeline.setInt("equirectangularMap", 0);

    glViewport(0, 0, 512, 512);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

    for (unsigned int i = 0; i < 6; i++) {
        convertToCubemapPipeline.setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemapTexture, 0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(cubemapBuffer.VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    prefilterPipeline.use();
    prefilterPipeline.setInt("environmentMap", 0);
    prefilterPipeline.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    unsigned int maxMipLevels = 5;
    for (unsigned int mip = 0; mip < maxMipLevels; mip++) {
        unsigned int mipWidth = 128 * std::pow(0.5, mip);
        unsigned int mipHeight = 128 * std::pow(0.5, mip);

        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip / (float)(maxMipLevels - 1);
        prefilterPipeline.setFloat("roughness", roughness);

        for (unsigned int i = 0; i < 6; i++) {
            prefilterPipeline.setMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                prefilterMap, mip);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glBindVertexArray(cubemapBuffer.VAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, irradianceRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

    createIrradiancePipeline.use();
    createIrradiancePipeline.setInt("enviornmentMap", 0);
    createIrradiancePipeline.setMat4("projection", captureProjection);
    glActiveTexture(0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

    glViewport(0, 0, 32, 32);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    for (unsigned int i = 0; i < 6; i++) {
        convertToCubemapPipeline.setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(cubemapBuffer.VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0, 0, 512, 512);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

    brdfPipeline.use();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindVertexArray(quadBuffer.VAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
}

void PBREngine::createPrefilter() {
    prefilterPipeline.use();

}

void PBREngine::run() {
    bool closedWindow = false;
    SDL_Event event;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    createIrradianceMap();

    while (!closedWindow) {
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        float currentFrame = static_cast<float>(SDL_GetTicks());
        deltaTime = currentFrame - lastFrame;
        deltaTime *= 0.1;
        lastFrame = currentFrame;

        while (SDL_PollEvent(&event)) {
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

                if (type == SDLK_SPACE) showModel = !showModel;
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

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        pipeline.use();

        pipeline.setVec3("albedo", glm::vec3(0.5f, 0.0f, 0.0f));
        pipeline.setFloat("ao", 1.0f);
        
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.getViewMatrix();

        pipeline.setMat4("projection", projection);
        pipeline.setMat4("view", view);
        pipeline.setVec3("viewPos", camera.Position);
        pipeline.setInt("irradianceMap", 0);
        pipeline.setInt("prefilterMap", 1);
        pipeline.setInt("brdfLUT", 2);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);

        for (unsigned int i = 0; i < lightPositions.size(); i++) {
            pipeline.setVec3("lights[" + std::to_string(i) + "].position", lightPositions[i]);
            pipeline.setVec3("lights[" + std::to_string(i) + "].color", lightColors[i]);
        }

        glm::mat4 model = glm::mat4(1.0f);
        if (showModel) {
            drawModels(pipeline, false);
        } else {
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, albedoMap);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, normalMap);
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, metallicMap);
            glActiveTexture(GL_TEXTURE6);
            glBindTexture(GL_TEXTURE_2D, aoMap);
            glActiveTexture(GL_TEXTURE7);
            glBindTexture(GL_TEXTURE_2D, roughnessMap);

            pipeline.setInt("texture_diffuse", 3);
            pipeline.setInt("texture_normal", 4);
            pipeline.setInt("texture_metallic", 5);
            pipeline.setInt("texture_ao", 6);
            pipeline.setInt("texture_roughness", 7);

            int nrRows    = 7;
            int nrColumns = 7;
            float spacing = 2.5;
            for (int row = 0; row < nrRows; row++) {
                pipeline.setFloat("metallic", (float)row / (float)nrRows);

                for (int col = 0; col < nrColumns; col++) {
                    pipeline.setFloat("roughness", glm::clamp((float) col / (float) nrColumns, 0.05f, 1.0f));

                    model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(
                        (col - (nrColumns / 2)) * spacing, 
                        (row - (nrRows / 2)) * spacing, 
                        0.0f
                    ));
                    pipeline.setMat4("model", model);

                    glBindVertexArray(sphereBuffer.VAO);
                    glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
                }
            }
        }

        for (unsigned int i = 0; i < sizeof(lightPositions) / sizeof(lightPositions[0]); ++i)
        {
            glm::vec3 newPos = lightPositions[i] + glm::vec3(sin(glfwGetTime() * 5.0) * 5.0, 0.0, 0.0);
            newPos = lightPositions[i];
            pipeline.setVec3("lights[" + std::to_string(i) + "].position", lightPositions[i]);
            pipeline.setVec3("lights[" + std::to_string(i) + "].color", lightColors[i]);

            model = glm::mat4(1.0f);
            model = glm::translate(model, newPos);
            model = glm::scale(model, glm::vec3(0.5f));
            pipeline.setMat4("model", model);
            glBindVertexArray(sphereBuffer.VAO);
            glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
        }

        backgroundPipeline.use();
        backgroundPipeline.setMat4("projection", projection);
        backgroundPipeline.setMat4("view", view);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        backgroundPipeline.setInt("environmentMap", 0);

        glBindVertexArray(cubemapBuffer.VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        SDL_GL_SwapWindow(window);
    }
}

void PBREngine::drawModels(Shader& shader, bool skipTextures) {
    for (Model& model : usableObjs) {
        shader.setMat4("model", model.model_matrix);

        for (int j = 0; j < model.meshes.size(); j++) {
            Mesh& mesh = model.meshes[j];
            if (!skipTextures) {
                unsigned int diffuseNr = 1;
                unsigned int specularNr = 1;
                unsigned int normalNr = 1;
                unsigned int heightNr = 1;
                unsigned int roughnessNr = 1;

                for (unsigned int i = 0; i < mesh.textures.size(); i++) {
                    glActiveTexture(GL_TEXTURE3 + i);

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

                    string key = name;
                    shader.setInt(key.c_str(), i + 3);

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