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
        std::string newBoneName = "meshIdx" + std::to_string(meshIdx);
        mBoneNameToIndexMap[newBoneName] = BoneIndex;

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
        printf("Bone Name: %s\n", BoneName.c_str());

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

void Skeleton::PopulateBuffer(unsigned int& vertexBuffer) const
{
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mBones[0]) * mBones.size(), &mBones[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, MAX_NUM_BONES_PER_VERTEX, GL_INT, sizeof(VertexBoneData), (const GLvoid*)0);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, MAX_NUM_BONES_PER_VERTEX, GL_FLOAT, GL_FALSE, sizeof(VertexBoneData),
        (const GLvoid*)(MAX_NUM_BONES_PER_VERTEX * sizeof(int32_t)));   // 後半4ByteがWeight

}

unsigned int Skeleton::GetBoneIdx(std::string boneName, bool& isFind) const
{
    auto iter = mBoneNameToIndexMap.find(boneName);
    if (iter != mBoneNameToIndexMap.end()) {
        isFind = true;
        return iter->second;
    }
    else {
        isFind = false;
        //printf("warn: this bone %s is not added to boneNameMap\n", boneName.c_str());
    }
    //assert(false);
}

glm::mat4 Skeleton::GetOffsetMatrix(std::string boneName) const
{
    bool isFind;
    return GetOffsetMatrix(GetBoneIdx(boneName, isFind));
}

glm::mat4 Skeleton::GetOffsetMatrix(int boneIdx) const
{
    assert(boneIdx < mOffsetMatrices.size());
    return mOffsetMatrices[boneIdx];
}
