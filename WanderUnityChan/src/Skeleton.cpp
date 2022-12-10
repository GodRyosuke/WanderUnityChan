#include "Skeleton.hpp"
#include "GLUtil.hpp"

void Skeleton::ReserveBoneSpace(unsigned int numVertices)
{
    mBones.resize(numVertices);
}

bool Skeleton::Load(const aiMesh* mesh, unsigned int meshIdx, unsigned int baseVertex)
{
    if (mesh->mNumBones == 0) {
        // Boneが割り当てられていないので、新たに作成
        int BoneIndex = (int)mBoneNameToIndexMap.size();
        mBoneNameToIndexMap[mesh->mName.C_Str()] = BoneIndex;

        for (int vertIdx = 0; vertIdx < mesh->mNumVertices; vertIdx++) {
            unsigned int GlobalVertexID = baseVertex + vertIdx;
            mBones[GlobalVertexID].AddBoneData(BoneIndex, 1.f);
        }

        if (BoneIndex == mOffsetMatrices.size()) {
            mOffsetMatrices.push_back(glm::mat4(1.f));
            //aiMatrix4x4 offsetMatrix = paiBone->mOffsetMatrix;
            //BoneInfo bi(glm::mat4(1.f));
            //m_BoneInfo.push_back(bi);
        }

        printf("warn: this mesh does not assigned bone: %s, meshIdx: %d\n", mesh->mName.C_Str(), meshIdx);

        return true;
    }

    for (unsigned int i = 0; i < mesh->mNumBones; i++) {
        aiBone* paiBone = mesh->mBones[i];

        // BoneIndexの取得
        int BoneIndex = 0;
        std::string BoneName = paiBone->mName.C_Str();
        if (mBoneNameToIndexMap.find(BoneName) == mBoneNameToIndexMap.end()) {
            // Allocate an index for a new bone
            BoneIndex = (int)mBoneNameToIndexMap.size();
            mBoneNameToIndexMap[BoneName] = BoneIndex;
        }
        else {
            BoneIndex = mBoneNameToIndexMap[BoneName];
        }

        if (BoneIndex == mOffsetMatrices.size()) {
            aiMatrix4x4 offsetMatrix = paiBone->mOffsetMatrix;
            //BoneInfo bi(GLUtil::ToGlmMat4(offsetMatrix));
            //m_BoneInfo.push_back(bi);
            mOffsetMatrices.push_back(GLUtil::ToGlmMat4(offsetMatrix));
        }

        // BoneのWeightを取得
        for (int weightIdx = 0; weightIdx < paiBone->mNumWeights; weightIdx++) {
            const aiVertexWeight& vw = paiBone->mWeights[weightIdx];
            unsigned int GlobalVertexID = baseVertex + paiBone->mWeights[weightIdx].mVertexId;
            //printf("vertexID:%d, BoneID:%d, weight: %f\n", GlobalVertexID, BoneIndex, vw.mWeight);
            mBones[GlobalVertexID].AddBoneData(BoneIndex, vw.mWeight);
        }
    }

    return true;
}
