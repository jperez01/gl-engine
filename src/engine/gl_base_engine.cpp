#include "gl_base_engine.h"
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

void GLEngine::init_resources() {}
void GLEngine::run() {}

void GLEngine::drawModels(Shader& shader, bool skipTextures) {
    for (Model& model : usableObjs) {

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

            glBindVertexArray(mesh.buffer.VAO);
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
        std::vector<VertexType> endpoints = {POSITION, NORMAL, TEXCOORDS, TANGENT, BI_TANGENT, VERTEX_ID};
        mesh.buffer = glutil::loadVertexBuffer(mesh.vertices, mesh.indices, endpoints);

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

void GLEngine::framebuffer_callback(int width, int height) {
    glViewport(0, 0, width, height);
    WINDOW_HEIGHT = height;
    WINDOW_WIDTH = width;
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

void GLEngine::handleClick(double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)WINDOW_WIDTH/ (float)WINDOW_HEIGHT, 0.1f, 100.0f);

    float convertedX = (2.0f * xpos) / WINDOW_WIDTH - 1.0f;
    float convertedY = (2.0f * ypos) / WINDOW_HEIGHT - 1.0f;
    glm::vec4 ray_clip(convertedX, convertedY, -1.0f, 1.0f);
    glm::vec4 ray_eye = glm::inverse(projection) * ray_clip;
    ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0f, 1.0f);
    glm::vec4 ray_world = glm::inverse(camera.getViewMatrix()) * ray_eye;

    glm::vec4 ray_origin = glm::vec4(camera.Position.x, camera.Position.y, camera.Position.z, 1.0f);
    glm::vec4 ray_dir = glm::normalize(ray_world - ray_origin);
    glm::vec4 inverse_dir = glm::vec4(1.0f) / ray_dir;

    checkIntersection(ray_origin, ray_dir, inverse_dir);
}

void GLEngine::checkIntersection(glm::vec4& origin, glm::vec4& direction, glm::vec4& inverse_dir) {
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