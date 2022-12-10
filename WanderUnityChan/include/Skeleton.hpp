// ----------------------------------------------------------------
// From Game Programming in C++ by Sanjay Madhav
// Copyright (C) 2017 Sanjay Madhav. All rights reserved.
// 
// Released under the BSD License
// See LICENSE in root directory for full details.
// ----------------------------------------------------------------

#pragma once
//#include "BoneTransform.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>       // Output data structure
#include <assimp/postprocess.h> // Post processing flags
#include "glm.hpp"

#define MAX_NUM_BONES_PER_VERTEX 4

class Skeleton
{
public:
    // Definition for each bone in the skeleton
    //struct Bone
    //{
    //    BoneTransform mLocalBindPose;
    //    std::string mName;
    //    int mParent;
    //};

    // Boneの読み出し
    bool Load(const aiMesh* mesh, unsigned int meshIdx, unsigned int baseVertex);
    void ReserveBoneSpace(unsigned int numVertices);

    void SetGlobalInvTrans(glm::mat4 globalInvTrans) { mGlobalInvTrans = globalInvTrans; }

    // Getter functions
    size_t GetNumBones() const { return mBones.size(); }
    //const Bone& GetBone(size_t idx) const { return mBones[idx]; }
    //const std::vector<Bone>& GetBones() const { return mBones; }
    //const std::vector<Matrix4>& GetGlobalInvBindPoses() const { return mGlobalInvBindPoses; }
protected:
    // Called automatically when the skeleton is loaded
    // Computes the global inverse bind pose for each bone
    void ComputeGlobalInvBindPose();
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

    // The bones in the skeleton
    //std::vector<Bone> mBones;
    // The global inverse bind poses for each bone
    //std::vector<Matrix4> mGlobalInvBindPoses;
    std::unordered_map<std::string, unsigned int> mBoneNameToIndexMap;
    std::vector<VertexBoneData> mBones;
    std::vector<glm::mat4> mOffsetMatrices;

    glm::mat4 mGlobalInvTrans;
};
