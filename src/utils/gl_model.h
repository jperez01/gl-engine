#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "stb_image.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>
#include <vector>
#include <unordered_map>

#include "gl_types.h"

bool textureFromFile(const char *path, const std::string &directory, Texture& texture, bool gamma = false);

class Model {
    public:
        std::vector<Texture> textures_loaded;
        std::vector<Mesh> meshes;
        std::string directory;
        bool gammaCorrection;
        glm::mat4 model_matrix;

        Model();
        Model(std::string path);
    private:
        void loadInfo(std::string path);

        void processNode(aiNode *node, const aiScene *scene);
        Mesh processMesh(aiMesh *mesh, const aiScene *scene);

        std::vector<std::string> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName);
};