#include "gl_model.h"

#include <iostream>
#include <glm/gtx/quaternion.hpp>

void Mesh::calcInterpolatedScaling(aiVector3D& out, float animationTicks, const aiNodeAnim* nodeAnim) {
    if (nodeAnim->mNumScalingKeys == 1) {
        out = nodeAnim->mScalingKeys[0].mValue;
        return;
    }

    unsigned int scalingIndex = 0;
    for (unsigned int i = 0; i < nodeAnim->mNumScalingKeys - 1; i++) {
        float t = nodeAnim->mScalingKeys[i+1].mTime;
        if (animationTicks < t)  {
            scalingIndex = i;
            break;
        }
    }
    unsigned int nextScalingIndex = scalingIndex + 1;

    float t1 = nodeAnim->mScalingKeys[scalingIndex].mTime,
        t2 = nodeAnim->mScalingKeys[nextScalingIndex].mTime;
    
    float deltaTime = t2 - t1;
    float factor = (animationTicks - t1) / deltaTime;

    const aiVector3D& start = nodeAnim->mScalingKeys[scalingIndex].mValue,
        end = nodeAnim->mScalingKeys[nextScalingIndex].mValue;
    aiVector3D delta = end - start;
    out = start + factor * delta;
}

void Mesh::calcInterpolatedRotation(aiQuaternion& out, float animationTicks, const aiNodeAnim* nodeAnim) {
    if (nodeAnim->mNumRotationKeys == 1) {
        out = nodeAnim->mRotationKeys[0].mValue;
        return;
    }

    unsigned int rotationIndex = 0;
    for (unsigned int i = 0; i < nodeAnim->mNumRotationKeys - 1; i++) {
        float t = nodeAnim->mRotationKeys[i+1].mTime;
        if (animationTicks < t)  {
            rotationIndex = i;
            break;
        }
    }
    unsigned int nextRotationIndex = rotationIndex + 1;
    assert(nextRotationIndex < nodeAnim->mNumRotationKeys);

    float t1 = nodeAnim->mRotationKeys[rotationIndex].mTime,
        t2 = nodeAnim->mRotationKeys[nextRotationIndex].mTime;
    
    float deltaTime = t2 - t1;
    float factor = (animationTicks - t1) / deltaTime;

    const aiQuaternion& start = nodeAnim->mRotationKeys[rotationIndex].mValue,
        end = nodeAnim->mRotationKeys[nextRotationIndex].mValue;
    
    aiQuaternion::Interpolate(out, start, end, factor);
    out.Normalize();
}

void Mesh::calcInterpolatedPosition(aiVector3D& out, float animationTicks, const aiNodeAnim* nodeAnim) {
    if (nodeAnim->mNumPositionKeys == 1) {
        out = nodeAnim->mPositionKeys[0].mValue;
        return;
    }

    unsigned int positionIndex = 0;
    for (unsigned int i = 0; i < nodeAnim->mNumPositionKeys - 1; i++) {
        float t = nodeAnim->mPositionKeys[i+1].mTime;
        if (animationTicks < t)  {
            positionIndex = i;
            break;
        }
    }
    unsigned int nextPositionIndex = positionIndex + 1;

    float t1 = nodeAnim->mPositionKeys[positionIndex].mTime,
        t2 = nodeAnim->mPositionKeys[nextPositionIndex].mTime;
    
    float deltaTime = t2 - t1;
    float factor = (animationTicks - t1) / deltaTime;

    const aiVector3D& start = nodeAnim->mPositionKeys[positionIndex].mValue,
        end = nodeAnim->mPositionKeys[nextPositionIndex].mValue;
    aiVector3D delta = end - start;
    out = start + factor * delta;
}

void Mesh::getBoneTransforms(float time, const aiScene* scene, 
    std::vector<NodeData>& nodeData, int animationIndex) {
    const aiAnimation* animation = scene->mAnimations[animationIndex];

    float ticksPerSecond = animation->mTicksPerSecond != 0 
        ? animation->mTicksPerSecond : 25.0f;
    float timeInTicks = time * ticksPerSecond;
    float animationTimeTicks = fmod(timeInTicks, animation->mDuration);

    glm::mat4 identity(1.0f);

    for (int i = 0; i < nodeData.size(); i++) {
        NodeData& node = nodeData[i];
        const aiNodeAnim* nodeAnim = findNodeAnim(animation, node.name);
        glm::mat4 totalTransform = node.transformation;

        if (nodeAnim) {
            aiVector3D scaling;
            calcInterpolatedScaling(scaling, animationTimeTicks, nodeAnim);
            glm::mat4 scalingMatrix = glm::scale(identity, glm::vec3(scaling.x, scaling.y, scaling.z));

            aiQuaternion rotationQ;
            calcInterpolatedRotation(rotationQ, animationTimeTicks, nodeAnim);
            glm::quat rotateQ(rotationQ.w, rotationQ.x, rotationQ.y, rotationQ.z);
            glm::mat4 rotationMatrix = glm::toMat4(rotateQ);

            aiVector3D translation;
            calcInterpolatedPosition(translation, animationTimeTicks, nodeAnim);
            glm::mat4 translationMatrix = glm::translate(identity, glm::vec3(translation.x, translation.y, translation.z));

            totalTransform = translationMatrix * rotationMatrix * scalingMatrix;
        }

        glm::mat4 parentTrasform = (node.parentIndex == -1) ? identity : nodeData[node.parentIndex].transformation;
        node.transformation = parentTrasform * totalTransform;

        if (boneName_To_Index.find(node.name) != boneName_To_Index.end()) {
            unsigned int boneIndex = boneName_To_Index[node.name];
            bone_info[boneIndex].finalTransform = globalInverse * node.transformation * bone_info[boneIndex].offsetTransform;
        }
    }
}

const aiNodeAnim* Mesh::findNodeAnim(const aiAnimation* animation, const std::string nodeName) {
    for (unsigned int i = 0; i < animation->mNumChannels; i++) {
        const aiNodeAnim* nodeAnim = animation->mChannels[i];

        if (std::string(nodeAnim->mNodeName.data) == nodeName) return nodeAnim;
    }
    return nullptr;
}

Model::Model() = default;

Model::Model(std::string path, FileType type) {
    loadInfo(path, type);
    model_matrix = glm::mat4(1.0f);
}

void Model::loadInfo(std::string path, FileType type) {
    int fileTypeInfo[2] = {
        aiProcess_ConvertToLeftHanded, 0
    };
    Assimp::Importer importer;
    scene = importer.ReadFile(path,
        aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace |
        fileTypeInfo[type]);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return;
    }
    directory = path.substr(0, path.find_last_of('/'));
    numAnimations = scene->mNumAnimations;

    processNode(scene->mRootNode, scene);
    scene = importer.GetOrphanedScene();
}

void Model::processNode(aiNode *node, const aiScene *scene, int parentIndex) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }

    NodeData data;
    data.name = std::string(node->mName.data);
    data.transformation = convertMatrix(node->mTransformation);
    data.parentIndex = parentIndex;
    nodes.push_back(data);
    int index = nodes.size() - 1;

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, index);
    }
}

Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<std::string> textures;
    std::vector<VertexBoneData> boneData;
    std::vector<BoneInfo> boneInfo;
    std::unordered_map<std::string, unsigned int> nameToIndex;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        vertex.ID = i;

        glm::vec3 vector;
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.Position = vector;

        if (!aabb.isInitialized) {
            aabb.isInitialized = true;
            aabb.minPoint = glm::vec4(vector, 1.0f);
            aabb.maxPoint = glm::vec4(vector, 1.0f);
        } else {
            aabb.minPoint = glm::vec4(
                std::min(aabb.minPoint.x, vector.x),
                std::min(aabb.minPoint.y, vector.y),
                std::min(aabb.minPoint.z, vector.z),
                1.0f
            );
            aabb.maxPoint = glm::vec4(
                std::max(aabb.maxPoint.x, vector.x),
                std::max(aabb.maxPoint.y, vector.y),
                std::max(aabb.maxPoint.z, vector.z),
                1.0f
            );
        }

        if (mesh->HasNormals()) {
            glm::vec3 normal;
            normal.x = mesh->mNormals[i].x;
            normal.y = mesh->mNormals[i].y;
            normal.z = mesh->mNormals[i].z;
            vertex.Normal = normal;
        }

        if (mesh->mTextureCoords[0]) {
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;

            vector.x = mesh->mTangents[i].x;
            vector.y = mesh->mTangents[i].y;
            vector.z = mesh->mTangents[i].z;
            vertex.Tangent = vector;

            vector.x = mesh->mBitangents[i].x;
            vector.y = mesh->mBitangents[i].y;
            vector.z = mesh->mBitangents[i].z;
            vertex.Bitangent = vector;
        } else {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }
        vertices.push_back(vertex);
    }

    if(mesh->HasBones()) {
        boneData.resize(vertices.size());
        boneInfo.resize(mesh->mNumBones);
        for (unsigned int i = 0; i < mesh->mNumBones; i++) {
            aiBone* bone = mesh->mBones[i];

            std::string bone_name(bone->mName.C_Str());
            nameToIndex[bone_name] = i;

            boneInfo[i].offsetTransform = convertMatrix(bone->mOffsetMatrix);
            for (unsigned int j = 0; j < bone->mNumWeights; j++) {
                auto& weight = bone->mWeights[j];
                addBoneData(boneData[weight.mVertexId], i, weight.mWeight);
            }
        }
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }
    aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

    std::vector<std::string> diffuseMaps = loadMaterialTextures(material,
        aiTextureType_DIFFUSE, "texture_diffuse");
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

    std::vector<std::string> specularMaps = loadMaterialTextures(material,
        aiTextureType_SPECULAR, "texture_specular");
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

    std::vector<std::string> normalMaps = loadMaterialTextures(material,
        aiTextureType_HEIGHT, "texture_normal");
    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

    std::vector<std::string> heightMaps = loadMaterialTextures(material,
        aiTextureType_AMBIENT, "texture_height");
    textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

    Mesh newMesh;
    newMesh.indices = indices;
    newMesh.vertices = vertices;
    newMesh.texture_paths = textures;
    newMesh.bone_data = boneData;
    newMesh.bone_info = boneInfo;
    newMesh.boneName_To_Index = nameToIndex;
    glm::mat4 globalTransform = convertMatrix(scene->mRootNode->mTransformation);
    newMesh.globalInverse = glm::inverse(globalTransform);
    
    return newMesh;
}

std::vector<std::string> Model::loadMaterialTextures(aiMaterial *mat, aiTextureType type, 
    std::string typeName) {
    std::vector<std::string> textures;

    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        bool skip = false;

        for (unsigned int j = 0; j < textures_loaded.size(); j++) {
            if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0) {
                textures.push_back(textures_loaded[j].path);
                skip = true;
                break;
            }
        }

        if (!skip) {
            Texture texture;
            bool success = false;

            const aiTexture* embeddedTexture = scene->GetEmbeddedTexture(str.C_Str());
            if (embeddedTexture) {
                success = textureFromMemory(embeddedTexture->pcData, embeddedTexture->mWidth, texture);
            }
            if (!success) {
                success = textureFromFile(str.C_Str(), directory, texture);
            }
            if (success) {
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture.path);
                textures_loaded.push_back(texture);
            }
        }
    }

    return textures;
}

bool textureFromMemory(void* data, unsigned int bufferSize, Texture& texture) {
    int width, height, nrComponents;
    unsigned char* image_data = stbi_load_from_memory((const stbi_uc*)data, bufferSize, &width, &height, &nrComponents, 0);

    if (image_data) {
        texture.data = image_data;
        texture.nrComponents = nrComponents;
        texture.width = width;
        texture.height = height;

        return true;
    } else {
        std::cout << "Embedded Texture failed to load " << std::endl;
        stbi_image_free(data);

        return false;
    }
}

bool textureFromFile(const char *path, const std::string &directory, Texture& texture, bool gamma) {
    std::string filename = std::string(path);
    filename = directory + '/' + filename;

    int width, height, nrComponents;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data) {
        texture.data = data;
        texture.nrComponents = nrComponents;
        texture.width = width;
        texture.height = height;
        return true;
    } else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);

        return false;
    }
}

glm::mat4 convertMatrix(const aiMatrix4x4& aiMat) {
    return {
        aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1,
        aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2,
        aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3,
        aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4
    };
}