#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "stb_image.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "shader.h"
#include "mesh.h"

#include <string>
#include <vector>
using namespace std;

struct ImportInfo {
    aiScene* scene;
    vector<char*> textureData;
};

unsigned int TextureFromFile(const char *path, const string &directory, bool gamma = false);

class Model {
    public:
        vector<Texture> textures_loaded;
        vector<Mesh> meshes;
        string directory;
        bool gammaCorrection;
        glm::mat4 model_matrix;

        Model();
        Model(string path);
        void draw(Shader &shader);
    private:
        void loadModel(string path);

        void processNode(aiNode *node, const aiScene *scene);
        Mesh processMesh(aiMesh *mesh, const aiScene *scene);

        vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName);
};
#endif