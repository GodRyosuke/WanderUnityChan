#pragma once

#include <vector>
#include <fbxsdk.h>
#include <glm.hpp>
#include <map>

#define MAX_NUM_BONES_PER_VERTEX 4

class FBXSkeleton {
public:
	FBXSkeleton();

	bool Load(FbxMesh* mesh, bool& hasSkin);
    void Update(float delatTime);
    void CreateVBO();
    void GetBoneIdexWeightArray(std::vector<glm::ivec4>& boneIndices,
        std::vector<glm::vec4>& boneWeights) {
        boneIndices = mBoneIndices;
        boneWeights = mBoneWeights;
    }
    void GetBoneMatrixPallete(std::vector<glm::mat4>& boneMatrices)
    {
        boneMatrices = mBoneMatrixPallete;
    }
    void GetBoneInvMatrixPallete(std::vector<glm::mat4>& boneMatrices)
    {
        boneMatrices = mBoneGlobalInvMatrices;
    }
    void GetBoneMatrixPallete(std::map<std::string, int>& boneMatrices)
    {
        boneMatrices = mBoneNameIdxTable;
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
            printf("Warning: more than 4 bones are assigned by vertex\n");
            //assert(0);
        }
    };

    glm::mat4 CopyFbxAMat(FbxAMatrix mat)
    {
        glm::mat4 outMat;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                outMat[i][j] = mat[i][j];
            }
        }
        return outMat;
    }

    glm::vec4 CopyFbxVec(FbxVector4 vec)
    {
        glm::vec4 Outvec;
        for (int i = 0; i < 4; i++) {
            Outvec[i] = vec[i];
        }
        return Outvec;
    }

    unsigned int mVertexBuffer;

    std::vector<VertexBoneData> mBones;
    // 各頂点に影響を与えるBoneの数
    std::vector<glm::ivec4> mBoneIndices;
    // 影響を与えるBoneのWeight
    std::vector<glm::vec4> mBoneWeights;
    std::vector<float> mBoneWeightSchalar;
    std::vector<glm::mat4> deBoneMatrixPallete;
    std::vector<glm::mat4> mBoneMatrixPallete;
    std::map<std::string, int> mBoneNameIdxTable;
    std::vector<glm::mat4> mBoneGlobalInvMatrices;
};
