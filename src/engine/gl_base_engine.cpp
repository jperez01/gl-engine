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

void GLEngine::drawModels(std::vector<Model>& models, Shader& shader, unsigned char drawOptions) {
    bool shouldSkipTextures = drawOptions & SKIP_TEXTURES;
    bool shouldSkipCulling = drawOptions & SKIP_CULLING;

    for (Model& model : models) {
        if (!shouldSkipCulling) {
            glm::vec4 transformedMax = model.model_matrix * model.aabb.maxPoint;
            glm::vec4 transformedMin = model.model_matrix * model.aabb.minPoint;
            bool shouldDraw = camera->isInsideFrustum(transformedMax, transformedMin);
            if (!shouldDraw) continue;
        }
        
        for (int j = 0; j < model.meshes.size(); j++) {
            Mesh& mesh = model.meshes[j];

            glm::mat4 finalModelMatrix = mesh.model_matrix * model.model_matrix;
            if (!shouldSkipCulling) {
                glm::vec4 meshMin = finalModelMatrix * mesh.aabb.minPoint;
                glm::vec4 meshMax = finalModelMatrix * mesh.aabb.maxPoint;
                bool shouldDraw = camera->isInsideFrustum(meshMax, meshMin);
                if (!shouldDraw) continue;
            }

            shader.setMat4("model", finalModelMatrix);
            if (!shouldSkipTextures) {
                Material material = model.materials_loaded[mesh.materialIndex];

                if (material.textures.size() != 4) {
                    shader.setBool("noMetallicMap", true);
                    shader.setBool("noNormalMap", true);
                } else {
                    shader.setBool("noMetallicMap", false);
                    shader.setBool("noNormalMap", false);
                }

                for (unsigned int i = 0; i < material.textures.size(); i++) {
                    glActiveTexture(GL_TEXTURE0 + i);

                    string number;
                    string name = material.textures[i].type;

                    string key = name;
                    shader.setInt(key.c_str(), i);

                    glBindTexture(GL_TEXTURE_2D, material.textures[i].id);
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
    for (auto& info : model.textures_loaded) {
        Texture& texture = info.second;
        int levels = (texture.type == "texture_normal" || texture.width < 16) ? 1 : 4;
        unsigned int textureID = glutil::createTexture(texture.width, texture.height,
            GL_UNSIGNED_BYTE, texture.nrComponents, texture.data, levels);
        
        texture.id = textureID;

        stbi_image_free(texture.data);
    }

    for (Material& material : model.materials_loaded) {
        for (std::string& path : material.texture_paths) {
            Texture& texture = model.textures_loaded[path];
            material.textures.push_back(texture);
        }
    }

    for (Mesh& mesh : model.meshes) {
        std::vector<VertexType> endpoints = {POSITION, NORMAL, TEXCOORDS, TANGENT, BI_TANGENT, VERTEX_ID};
        mesh.buffer = glutil::loadVertexBuffer(mesh.vertices, mesh.indices, endpoints);
        
        if (mesh.bone_data.size() != 0 && model.scene->mAnimations > 0) {
            glCreateBuffers(1, &mesh.SSBO);
            glNamedBufferStorage(mesh.SSBO, sizeof(VertexBoneData) * mesh.bone_data.size(),
                mesh.bone_data.data(), GL_DYNAMIC_STORAGE_BIT);
        }
    }
}