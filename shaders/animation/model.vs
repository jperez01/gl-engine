#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 5) in uint id;

const int MAX_BONES_PER_VERTEX  = 4;
const int MAX_BONES = 200;

struct BoneData {
    uint boneIDs[MAX_BONES_PER_VERTEX];
    float weights[MAX_BONES_PER_VERTEX];
};

layout(std430, binding = 3) buffer boneData {
    BoneData data[];
};

out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 boneMatrices[MAX_BONES];

void main()
{
    TexCoords = aTexCoords;
    Normal = mat3(transpose(inverse(model))) * aNormal;

    BoneData vertexData = data[id];
    mat4 boneTransform = mat4(0.0f);
    for (int i = 0; i < MAX_BONES_PER_VERTEX; i++) {
        boneTransform += boneMatrices[vertexData.boneIDs[i]] * vertexData.weights[i];
    }

    vec4 posWithBone = boneTransform * vec4(aPos, 1.0);
    FragPos = vec3(model * posWithBone);
    gl_Position = projection * view * model * posWithBone;
}