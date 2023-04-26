#include "gl_base_engine.h"
#include "utils/gl_funcs.h"

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
#include "ImGuizmo.h"

void GLEngine::init_resources() {}
void render(std::vector<Model>& objs) {}

void GLEngine::drawModels(std::vector<Model>& models, Shader& shader, bool skipTextures) {
    for (Model& model : models) {
        if (!model.shouldDraw) continue;
        shader.setMat4("model", model.model_matrix);

        for (int j = 0; j < model.meshes.size(); j++) {
            Mesh& mesh = model.meshes[j];
            if (!skipTextures) {

                if (mesh.textures.size() != 4) {
                    shader.setBool("noMetallicMap", true);
                    shader.setBool("noNormalMap", true);
                } else {
                    shader.setBool("noMetallicMap", false);
                    shader.setBool("noNormalMap", false);
                }

                for (unsigned int i = 0; i < mesh.textures.size(); i++) {
                    glActiveTexture(GL_TEXTURE0 + i);

                    string number;
                    string name = mesh.textures[i].type;

                    string key = name;
                    shader.setInt(key.c_str(), i);

                    glBindTexture(GL_TEXTURE_2D, mesh.textures[i].id);
                }
                glActiveTexture(GL_TEXTURE0);

                if (mesh.bone_data.size() != 0 && model.scene->mAnimations > 0) {
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

void GLEngine::loadModelData(Model& model) {
    for (Texture& texture : model.textures_loaded) {
        unsigned int textureID = glutil::createTexture(texture.width, texture.height,
            GL_UNSIGNED_BYTE, texture.nrComponents, texture.data);
        glGenerateTextureMipmap(textureID);
        
        texture.id = textureID;

        stbi_image_free(texture.data);
    }

    for (Mesh& mesh : model.meshes) {
        std::vector<VertexType> endpoints = {POSITION, NORMAL, TEXCOORDS, TANGENT, BI_TANGENT, VERTEX_ID};
        mesh.buffer = glutil::loadVertexBuffer(mesh.vertices, mesh.indices, endpoints);
        
        if (mesh.bone_data.size() != 0 && model.scene->mAnimations > 0) {
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