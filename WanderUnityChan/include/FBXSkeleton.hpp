#pragma once

#include <vector>
#include <fbxsdk.h>
#include <glm.hpp>

#define MAX_NUM_BONES_PER_VERTEX 4

class FBXSkeleton {
public:
	FBXSkeleton();

	bool Load(FbxMesh* mesh);
    void GetBoneIdexWeightArray(std::vector<glm::ivec4>& boneIndices,
        std::vector<glm::vec4>& boneWeights) {
        boneIndices = mBoneIndices;
        boneWeights = mBoneWeights;
    }
    void DeleteBoneData() {
        mBoneIndices.clear();
        mBoneWeights.clear();
    }

private:
    struct VertexBoneData
    {
        unsigned int BoneIDs[MAX_NUM_BONES_PER_VERTEX] = { 0 };
        float Weights[MAX_NUM_BONES_PER_VERTEX] = { 0.0f };

        VertexBoneData()
        {
        }

        void AddBoneData(unsigned int BoneID, float Weight)
        {
            for (unsigned int i = 0; i < MAX_NUM_BONES_PER_VERTEX; i++) {
                //if ((BoneIDs[i] == BoneID) && (Weights[i] != 0.0)) { // すでに所持していたら追加しない
                //    return;
                //}
                if (Weights[i] == 0.0) {
                    BoneIDs[i] = BoneID;
                    Weights[i] = Weight;
                    //printf("Adding bone %d weight %f at index %i\n", BoneID, Weight, i);
                    return;
                }
            }

            // should never get here - more bones than we have space for
            //assert(0);
        }
    };


    std::vector<VertexBoneData> mBones;
    // 各頂点に影響を与えるBoneの数
    std::vector<glm::ivec4> mBoneIndices;
    // 影響を与えるBoneのWeight
    std::vector<glm::vec4> mBoneWeights;
};
