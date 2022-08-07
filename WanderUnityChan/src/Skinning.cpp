#include "Skinning.hpp"
#include <iostream>
#include <GLUtil.hpp>

//MeshSkinningAssimp::MeshSkinningAssimp(std::string ObjFilePath, std::string MtlFilePath, Shader* shader)
//    :MeshAssimp(ObjFilePath, MtlFilePath, shader)
//{
//
//}

SkinMesh::SkinMesh()
    :Mesh()
{

}

void SkinMesh::GetGlobalInvTrans()
{
    GLUtil glutil;
    m_GlobalInverseTransform = glutil.ToGlmMat4(m_pScene->mRootNode->mTransformation);
    m_GlobalInverseTransform = glm::inverse(m_GlobalInverseTransform);
}

void SkinMesh::ReserveVertexSpace()
{
    Mesh::ReserveVertexSpace();
    m_Bones.resize(mNumVertices);
}

void SkinMesh::LoadMesh(const aiMesh* pMesh, unsigned int meshIdx)
{
    Mesh::LoadMesh(pMesh, meshIdx);

    // Bone�̓ǂݍ���
    for (unsigned int i = 0; i < pMesh->mNumBones; i++) {
        aiBone* paiBone = pMesh->mBones[i];

        // BoneIndex�̎擾
        int BoneIndex = 0;
        std::string BoneName = paiBone->mName.C_Str();
        if (m_BoneNameToIndexMap.find(BoneName) == m_BoneNameToIndexMap.end()) {
            // Allocate an index for a new bone
            BoneIndex = (int)m_BoneNameToIndexMap.size();
            m_BoneNameToIndexMap[BoneName] = BoneIndex;
        }
        else {
            BoneIndex = m_BoneNameToIndexMap[BoneName];
        }

        if (BoneIndex == m_BoneInfo.size()) {
            GLUtil glutil;
            aiMatrix4x4 offsetMatrix = paiBone->mOffsetMatrix;
            BoneInfo bi(glutil.ToGlmMat4(offsetMatrix));
            m_BoneInfo.push_back(bi);
        }

        // Bone��Weight���擾
        for (int weightIdx = 0; weightIdx < paiBone->mNumWeights; weightIdx++) {
            const aiVertexWeight& vw = paiBone->mWeights[weightIdx];
            unsigned int GlobalVertexID = m_Meshes[meshIdx].BaseVertex + paiBone->mWeights[weightIdx].mVertexId;
            //printf("vertexID:%d, BoneID:%d, weight: %f\n", GlobalVertexID, BoneIndex, vw.mWeight);
            m_Bones[GlobalVertexID].AddBoneData(BoneIndex, vw.mWeight);
        }
    }
}

void SkinMesh::PopulateBuffers()
{
    GLuint m_boneBuffer;
    // ���_�f�[�^��ǂݍ���
    Mesh::PopulateBuffers();

    // Bone and Weights
    glGenBuffers(1, &m_boneBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_boneBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(m_Bones[0]) * m_Bones.size(), &m_Bones[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, MAX_NUM_BONES_PER_VERTEX, GL_INT, sizeof(VertexBoneData), (const GLvoid*)0);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, MAX_NUM_BONES_PER_VERTEX, GL_FLOAT, GL_FALSE, sizeof(VertexBoneData),
        (const GLvoid*)(MAX_NUM_BONES_PER_VERTEX * sizeof(int32_t)));   // �㔼4Byte��Weight
}


void SkinMesh::CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTimeTicks, const aiNodeAnim* pNodeAnim)
{
    // we need at least two values to interpolate...
    if (pNodeAnim->mNumRotationKeys == 1) {
        Out = pNodeAnim->mRotationKeys[0].mValue;
        return;
    }

    assert(pNodeAnim->mNumRotationKeys > 0);

    // ���݂̃t���[����Rtation Matrix��ǂ݂���
    unsigned int RotationIndex = 0;
    for (unsigned int i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++) {
        float t = (float)pNodeAnim->mRotationKeys[i + 1].mTime;
        if (AnimationTimeTicks < t) {
            RotationIndex = i;
            break;
        }
    }
    unsigned int NextRotationIndex = RotationIndex + 1;
    assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);
    float t1 = (float)pNodeAnim->mRotationKeys[RotationIndex].mTime;
    if (t1 > AnimationTimeTicks) {
        Out = pNodeAnim->mRotationKeys[RotationIndex].mValue;
    }
    else {
        float t2 = (float)pNodeAnim->mRotationKeys[NextRotationIndex].mTime;
        float DeltaTime = t2 - t1;
        float Factor = (AnimationTimeTicks - t1) / DeltaTime;
        assert(Factor >= 0.0f && Factor <= 1.0f);
        const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
        const aiQuaternion& EndRotationQ = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;
        aiQuaternion::Interpolate(Out, StartRotationQ, EndRotationQ, Factor);
        Out = StartRotationQ;
    }

    Out.Normalize();
}

void SkinMesh::CalcInterpolatedScaling(aiVector3D& Out, float AnimationTimeTicks, const aiNodeAnim* pNodeAnim)
{
    // we need at least two values to interpolate...
    if (pNodeAnim->mNumScalingKeys == 1) {
        Out = pNodeAnim->mScalingKeys[0].mValue;
        return;
    }

    assert(pNodeAnim->mNumScalingKeys > 0);

    // ���݂̃t���[����Scaling Matrix��ǂ݂���
    unsigned int ScalingIndex = 0;
    for (unsigned int i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++) {
        float t = (float)pNodeAnim->mScalingKeys[i + 1].mTime;
        if (AnimationTimeTicks < t) {
            ScalingIndex = i;
            break;
        }
    }

    unsigned int NextScalingIndex = ScalingIndex + 1;
    assert(NextScalingIndex < pNodeAnim->mNumScalingKeys);
    float t1 = (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime;
    if (t1 > AnimationTimeTicks) {
        Out = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
    }
    else {
        // ���ݎ������t���[���ƃt���[���̊Ԃł���Ε⊮����
        float t2 = (float)pNodeAnim->mScalingKeys[NextScalingIndex].mTime;
        float DeltaTime = t2 - t1;
        float Factor = (AnimationTimeTicks - (float)t1) / DeltaTime;
        assert(Factor >= 0.0f && Factor <= 1.0f);
        const aiVector3D& Start = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
        const aiVector3D& End = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;
        aiVector3D Delta = End - Start;
        Out = Start + Factor * Delta;
    }
}

void SkinMesh::CalcInterpolatedPosition(aiVector3D& Out, float AnimationTimeTicks, const aiNodeAnim* pNodeAnim)
{
    // we need at least two values to interpolate...
    if (pNodeAnim->mNumPositionKeys == 1) {
        Out = pNodeAnim->mPositionKeys[0].mValue;
        return;
    }


    // ���݂̃t���[����Position Matrix��ǂ݂���
    unsigned int PositionIndex = 0;
    for (unsigned int i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++) {
        float t = (float)pNodeAnim->mPositionKeys[i + 1].mTime;
        if (AnimationTimeTicks < t) {
            PositionIndex = i;
            break;
        }
    }
    unsigned int NextPositionIndex = PositionIndex + 1;
    assert(NextPositionIndex < pNodeAnim->mNumPositionKeys);
    float t1 = (float)pNodeAnim->mPositionKeys[PositionIndex].mTime;
    if (t1 > AnimationTimeTicks) {
        Out = pNodeAnim->mPositionKeys[PositionIndex].mValue;
    }
    else {
        float t2 = (float)pNodeAnim->mPositionKeys[NextPositionIndex].mTime;
        float DeltaTime = t2 - t1;
        float Factor = (AnimationTimeTicks - t1) / DeltaTime;
        assert(Factor >= 0.0f && Factor <= 1.0f);
        const aiVector3D& Start = pNodeAnim->mPositionKeys[PositionIndex].mValue;
        const aiVector3D& End = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;
        aiVector3D Delta = End - Start;
        Out = Start + Factor * Delta;
    }
}

const aiAnimation* SkinMesh::SetAnimPointer()
{
    return m_pScene->mAnimations[0];
}


void SkinMesh::ReadNodeHierarchy(float AnimationTimeTicks, const aiNode* pNode, const glm::mat4& ParentTransform)
{
    GLUtil glutil;
    std::string NodeName(pNode->mName.data);
    int x = 0;

    const aiAnimation* pAnimation = SetAnimPointer();

    // Node�̎���Transform
    glm::mat4 NodeTransformation;
    {
        aiMatrix4x4 trans = pNode->mTransformation;
        NodeTransformation = glutil.ToGlmMat4(trans);
    }

    // ���݂�Node��Animation Data��ǂ݂���
    const aiNodeAnim* pNodeAnim = NULL;
    for (unsigned int i = 0; i < pAnimation->mNumChannels; i++) {
        const aiNodeAnim* nodeAnim = pAnimation->mChannels[i];

        if (std::string(nodeAnim->mNodeName.data) == NodeName) {
            pNodeAnim = nodeAnim;
        }
    }

    // ����Node��Animation������΁A
    if (pNodeAnim) {
        // ���ݎ�����Animation Transform���������킹��B
        // Interpolate scaling and generate scaling transformation matrix
        aiVector3D Scaling;
        CalcInterpolatedScaling(Scaling, AnimationTimeTicks, pNodeAnim);
        glm::vec3 scaleVec = glm::vec3(Scaling.x, Scaling.y, Scaling.z);
        glm::mat4 ScalingM = glm::scale(glm::mat4(1.0f), scaleVec);

        // Interpolate rotation and generate rotation transformation matrix
        aiQuaternion RotationQ;
        CalcInterpolatedRotation(RotationQ, AnimationTimeTicks, pNodeAnim);
        //Matrix4f RotationM = Matrix4f(RotationQ.GetMatrix());
        aiMatrix3x3 rotationMat = RotationQ.GetMatrix();
        glm::mat4 RotationM = glutil.ToGlmMat4(rotationMat);
        //std::cout << RotationM[0][0] << '\t' << RotationM[0][1] << '\t' << RotationM[0][2] << '\t' << RotationM[0][3] << '\t' << std::endl;

        // Interpolate translation and generate translation transformation matrix
        aiVector3D Translation;
        CalcInterpolatedPosition(Translation, AnimationTimeTicks, pNodeAnim);
        glm::mat4 TranslationM = glm::translate(glm::mat4(1.0f), glm::vec3(Translation.x, Translation.y, Translation.z));

        // Combine the above transformations
        NodeTransformation = TranslationM * RotationM * ScalingM;
    }

    glm::mat4 GlobalTransformation = ParentTransform * NodeTransformation;

    if (m_BoneNameToIndexMap.find(NodeName) != m_BoneNameToIndexMap.end()) {
        unsigned int BoneIndex = m_BoneNameToIndexMap[NodeName];
        m_BoneInfo[BoneIndex].FinalTransformation = m_GlobalInverseTransform * GlobalTransformation * m_BoneInfo[BoneIndex].OffsetMatrix;
    }

    for (unsigned int i = 0; i < pNode->mNumChildren; i++) {
        ReadNodeHierarchy(AnimationTimeTicks, pNode->mChildren[i], GlobalTransformation);
    }
}

void SkinMesh::GetBoneTransform(float TimeInSeconds, std::vector<glm::mat4>& Transforms)
{
    int num = m_pScene->mNumAnimations;
    if (num == 0) {
        Transforms.resize(m_BoneInfo.size());
        for (unsigned int i = 0; i < m_BoneInfo.size(); i++) {
            Transforms[i] = glm::mat4(1.0f);
        }
        return;
    }

    float TicksPerSecond = (float)(m_pScene->mAnimations[0]->mTicksPerSecond != NULL ? m_pScene->mAnimations[0]->mTicksPerSecond : 25.0f);
    float TimeInTicks = TimeInSeconds * TicksPerSecond;
    float Duration = 0.0f;  // Animation��Duration�̐�������������
    float fraction = modf((float)m_pScene->mAnimations[0]->mDuration, &Duration);
    float AnimationTimeTicks = fmod(TimeInTicks, Duration);



    glm::mat4 Identity = glm::mat4(1);
    // Node�̊K�w�\���ɂ��������āAAnimationTicks�����ɂ�����eBone��Transform�����߂�
    ReadNodeHierarchy(AnimationTimeTicks, m_pScene->mRootNode, Identity);
    Transforms.resize(m_BoneInfo.size());

    for (unsigned int i = 0; i < m_BoneInfo.size(); i++) {
        Transforms[i] = m_BoneInfo[i].FinalTransformation;
    }
}

void SkinMesh::UpdateBoneTransform(Shader* shader, float TimeInSeconds)
{
    // ���ݎ�����Bone Transform���擾
    std::vector<glm::mat4> BoneMatrixPalete;
    GetBoneTransform(TimeInSeconds, BoneMatrixPalete);

    // Shader�ɓn��
    for (int i = 0; i < BoneMatrixPalete.size(); i++) {
        std::string uniformName = "uMatrixPalette[" + std::to_string(i) + ']';
        shader->SetMatrixUniform(uniformName, BoneMatrixPalete[i]);
    }
}

void SkinMesh::UpdateTransform(Shader* shader, float timeInSeconds)
{
    Mesh::UpdateTransform(shader, timeInSeconds);
    UpdateBoneTransform(shader, timeInSeconds);
}

