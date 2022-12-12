#pragma once
#include <string>
#include <glm.hpp>
#include <vector>
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>       // Output data structure
#include <assimp/postprocess.h> // Post processing flags

#define ASSIMP_LOAD_FLAGS (aiProcess_Triangulate | aiProcess_GenSmoothNormals |  aiProcess_JoinIdenticalVertices )

class Animation
{
public:
    Animation();
    bool Load(std::string filePath);
    // Boneを割り当てられていないMeshでtransformを指定する場合
    bool Load(std::string filePath, glm::mat4 meshMat);
    void Update(float timeInSeconds);
    void GetGlobalPoseAtTime(std::vector<glm::mat4>& outPoses, const class Skeleton* inSkeleton, float inTime, int animIdx) const;

    void SetAnimIndex(int index);
    float GetDuration(int animIdx) const;
    float GetAnimTicks(float inTime, int animIdx) const;

private:
    void ReadNodeHierarchy(const aiAnimation* anim, const class Skeleton* inSkeleton, float AnimationTimeTicks, const aiNode* pNode, const glm::mat4& ParentTransform, std::vector<glm::mat4>& poses) const;
    void CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTimeTicks, const aiNodeAnim* pNodeAnim) const;
    void CalcInterpolatedScaling(aiVector3D& Out, float AnimationTimeTicks, const aiNodeAnim* pNodeAnim) const;
    void CalcInterpolatedPosition(aiVector3D& Out, float AnimationTimeTicks, const aiNodeAnim* pNodeAnim) const;


    //const class SkinMesh* mSkinMesh;
    const aiScene* m_pScene;
    Assimp::Importer m_Importer;
    bool mIsSetMeshMat;
    glm::mat4 mMeshMat;
    //glm::mat4 m_GlobalInverseTransform;
    //float mAnimDuration;
};
