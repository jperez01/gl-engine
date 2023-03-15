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
#include <memory>

#include "gl_types.h"

struct NodeData {
    glm::mat4 transformation;
    glm::mat4 originalTransform;
    std::string name;
    int parentIndex;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    std::vector<Texture> textures;
    std::vector<std::string> texture_paths;

    std::unordered_map<std::string, unsigned int> boneName_To_Index;
    std::vector<VertexBoneData> bone_data;
    std::vector<BoneInfo> bone_info;

    glm::mat4 globalInverse;

    AllocatedBuffer buffer;
    unsigned int SSBO;

    void getBoneTransforms(float time, const aiScene* scene, std::vector<NodeData>& nodeData, int animationIndex = 0);
    const aiNodeAnim* findNodeAnim(const aiAnimation* animation, const std::string nodeName);

    void calcInterpolatedScaling(aiVector3D& out, float animationTicks, const aiNodeAnim* nodeAnim);
    void calcInterpolatedRotation(aiQuaternion& out, float animationTicks, const aiNodeAnim* nodeAnim);
    void calcInterpolatedPosition(aiVector3D& out, float animationTicks, const aiNodeAnim* nodeAnim);
};

enum FileType {
    GLTF = 0, OBJ
};

bool textureFromMemory(void* data, unsigned int bufferSize, Texture& texture);
bool textureFromFile(const char *path, const std::string &directory, Texture& texture, bool gamma = false);
glm::mat4 convertMatrix(const aiMatrix4x4& aiMat);

class Model {
    public:
        std::vector<Texture> textures_loaded;
        std::vector<Mesh> meshes;
        std::vector<NodeData> nodes;

        std::string directory;
        bool gammaCorrection;
        glm::mat4 model_matrix;
        BoundingBox aabb;
        bool shouldDraw = true;
        int numAnimations = 0;

        const aiScene* scene;

        Model();
        Model(std::string path, FileType type = OBJ);
    private:
        void loadInfo(std::string path, FileType type);

        void processNode(aiNode *node, const aiScene *scene, int parentIndex = -1);
        Mesh processMesh(aiMesh *mesh, const aiScene *scene);

        void readNodeHierarchy(const aiNode* node, Mesh& mesh);

        std::vector<std::string> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName);
};