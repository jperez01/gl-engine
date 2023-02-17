#include "gl_types.h"

void addBoneData(VertexBoneData& data, unsigned int boneID, float weight) {
    for (unsigned int i = 0; i < MAX_BONES_PER_VERTEX; i++) {
        if (data.boneIDs[i] == boneID) return;
    }
    if (weight == 0.0f) return;

    for (unsigned int i = 0; i < MAX_BONES_PER_VERTEX; i++) {
        if (data.weights[i] == 0.0f) {
            data.boneIDs[i] = boneID;
            data.weights[i] = weight;
            return;
        }
    }
}