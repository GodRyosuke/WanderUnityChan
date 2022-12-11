#include "Animation.hpp"
#include "GLUtil.hpp"
#include "Skeleton.hpp"

bool Animation::Load(std::string filePath)
{
    m_pScene = m_Importer.ReadFile(filePath.c_str(), ASSIMP_LOAD_FLAGS);
    //m_pScene = pScene;

    if (!m_pScene) {
        printf("Error parsing '%s': '%s'\n", filePath.c_str(), m_Importer.GetErrorString());
        return false;
    }

    //m_GlobalInverseTransform = GLUtil::ToGlmMat4(m_pScene->mRootNode->mTransformation);
    //m_GlobalInverseTransform = glm::inverse(m_GlobalInverseTransform);

    int num = m_pScene->mNumAnimations;
    assert(num);
}

void Animation::SetAnimIndex(int index)
{
    float Duration = 0.0f;  // AnimationのDurationの整数部分が入る
    float fraction = modf((float)m_pScene->mAnimations[index]->mDuration, &Duration);
    mAnimDuration = Duration;
}


void Animation::CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTimeTicks, const aiNodeAnim* pNodeAnim) const
{
    // we need at least two values to interpolate...
    if (pNodeAnim->mNumRotationKeys == 1) {
        Out = pNodeAnim->mRotationKeys[0].mValue;
        return;
    }

    assert(pNodeAnim->mNumRotationKeys > 0);

    // 現在のフレームのRtation Matrixを読みだす
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
        if (!(Factor >= 0.0f && Factor <= 1.0f)) {
            //printf("warn: factor out of range\n");
            Factor = 1.f;
        }
        //assert(Factor >= 0.0f && Factor <= 1.0f);
        const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
        const aiQuaternion& EndRotationQ = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;
        aiQuaternion::Interpolate(Out, StartRotationQ, EndRotationQ, Factor);
        Out = StartRotationQ;
    }

    Out.Normalize();
}

void Animation::CalcInterpolatedScaling(aiVector3D& Out, float AnimationTimeTicks, const aiNodeAnim* pNodeAnim) const
{
    // we need at least two values to interpolate...
    if (pNodeAnim->mNumScalingKeys == 1) {
        Out = pNodeAnim->mScalingKeys[0].mValue;
        return;
    }

    assert(pNodeAnim->mNumScalingKeys > 0);

    // 現在のフレームのScaling Matrixを読みだす
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
        // 現在時刻がフレームとフレームの間であれば補完処理
        float t2 = (float)pNodeAnim->mScalingKeys[NextScalingIndex].mTime;
        float DeltaTime = t2 - t1;
        float Factor = (AnimationTimeTicks - (float)t1) / DeltaTime;
        if (!(Factor >= 0.0f && Factor <= 1.0f)) {
            //printf("warn: factor out of range\n");
            Factor = 1.f;
        }
        //assert(Factor >= 0.0f && Factor <= 1.0f);
        const aiVector3D& Start = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
        const aiVector3D& End = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;
        aiVector3D Delta = End - Start;
        Out = Start + Factor * Delta;
    }
}

void Animation::CalcInterpolatedPosition(aiVector3D& Out, float AnimationTimeTicks, const aiNodeAnim* pNodeAnim) const
{
    // we need at least two values to interpolate...
    if (pNodeAnim->mNumPositionKeys == 1) {
        Out = pNodeAnim->mPositionKeys[0].mValue;
        return;
    }

    // 現在のフレームのPosition Matrixを読みだす
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
        if (!(Factor >= 0.0f && Factor <= 1.0f)) {
            //printf("warn: factor out of range\n");
            Factor = 1.f;
        }
        //assert(Factor >= 0.0f && Factor <= 1.0f);
        const aiVector3D& Start = pNodeAnim->mPositionKeys[PositionIndex].mValue;
        const aiVector3D& End = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;
        aiVector3D Delta = End - Start;
        Out = Start + Factor * Delta;
    }
}

void Animation::ReadNodeHierarchy(const aiAnimation* anim, const Skeleton* inSkeleton, float AnimationTimeTicks, const aiNode* pNode, const glm::mat4& ParentTransform, std::vector<glm::mat4>& poses) const
{
    std::string NodeName(pNode->mName.data);
    for (int i = 0; i < pNode->mNumMeshes; i++) {
        unsigned int meshIdx = pNode->mMeshes[i];
        printf("meshIdx: %d\n", meshIdx);
    }


    // Nodeの持つTransform
    glm::mat4 NodeTransformation;
    {
        aiMatrix4x4 trans = pNode->mTransformation;
        NodeTransformation = GLUtil::ToGlmMat4(trans);
    }

    // 現在のNodeのAnimation Dataを読みだす
    const aiNodeAnim* pNodeAnim = NULL;
    for (unsigned int i = 0; i < anim->mNumChannels; i++) {
        const aiNodeAnim* nodeAnim = anim->mChannels[i];

        if (std::string(nodeAnim->mNodeName.data) == NodeName) {
            pNodeAnim = nodeAnim;
        }
    }

    // そのNodeにAnimationがあれば、
    if (pNodeAnim) {
        // 現在時刻のAnimation Transformをかけあわせる。
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
        glm::mat4 RotationM = GLUtil::ToGlmMat4(rotationMat);
        //std::cout << RotationM[0][0] << '\t' << RotationM[0][1] << '\t' << RotationM[0][2] << '\t' << RotationM[0][3] << '\t' << std::endl;

        // Interpolate translation and generate translation transformation matrix
        aiVector3D Translation;
        CalcInterpolatedPosition(Translation, AnimationTimeTicks, pNodeAnim);
        glm::mat4 TranslationM = glm::translate(glm::mat4(1.0f), glm::vec3(Translation.x, Translation.y, Translation.z));

        // Combine the above transformations
        NodeTransformation = TranslationM * RotationM * ScalingM;
    }
    printf("node name: %s\n", pNode->mName.C_Str());

    glm::mat4 GlobalTransformation = ParentTransform * NodeTransformation;

    unsigned int boneIndex = inSkeleton->GetBoneIdx(NodeName);

    // Derive Final Bone Transform
    glm::mat4 offsetMat = inSkeleton->GetOffsetMatrix(boneIndex);
    poses[boneIndex] = inSkeleton->GetGlobalInvTrans() * GlobalTransformation * offsetMat;
    //if (m_BoneNameToIndexMap.find(NodeName) != m_BoneNameToIndexMap.end()) {
    //    unsigned int BoneIndex = m_BoneNameToIndexMap[NodeName];
    //    m_BoneInfo[BoneIndex].FinalTransformation = m_GlobalInverseTransform * GlobalTransformation * m_BoneInfo[BoneIndex].OffsetMatrix;
    //}
    //else {
    //    assert(false);
    //}

    for (unsigned int i = 0; i < pNode->mNumChildren; i++) {
        ReadNodeHierarchy(anim, inSkeleton, AnimationTimeTicks, pNode->mChildren[i], GlobalTransformation, poses);
    }
}

void Animation::GetGlobalPoseAtTime(std::vector<glm::mat4>& outPoses, const Skeleton* inSkeleton, float inTime, int animIdx) const
{
    const int numBones = inSkeleton->GetNumBones();
    if (outPoses.size() != numBones)
    {
        outPoses.resize(numBones);
    }

    float TicksPerSecond = (float)(m_pScene->mAnimations[animIdx]->mTicksPerSecond != NULL ? m_pScene->mAnimations[animIdx]->mTicksPerSecond : 25.0f);
    float TimeInTicks = inTime * TicksPerSecond;
    float AnimationTimeTicks = fmod(TimeInTicks, mAnimDuration);

    glm::mat4 Identity = glm::mat4(1);
    // Nodeの階層構造にしたがって、AnimationTicks時刻における各BoneのTransformを求める
    aiAnimation* anim = m_pScene->mAnimations[animIdx];
    ReadNodeHierarchy(anim, inSkeleton, AnimationTimeTicks, m_pScene->mRootNode, Identity, outPoses);
    //Transforms.resize(numBones);

    //for (unsigned int i = 0; i < numBones; i++) {
    //    outPoses[i] = m_BoneInfo[i].FinalTransformation;
    //}


    // Figure out the current frame index and next frame
    // (This assumes inTime is bounded by [0, AnimDuration]
    //size_t frame = static_cast<size_t>(inTime / mFrameDuration);
    //size_t nextFrame = frame + 1;
    //// Calculate fractional value between frame and next frame
    //float pct = inTime / mFrameDuration - frame;

    //// Setup the pose for the root
    //if (mTracks[0].size() > 0)
    //{
    //    // Interpolate between the current frame's pose and the next frame
    //    BoneTransform interp = BoneTransform::Interpolate(mTracks[0][frame],
    //        mTracks[0][nextFrame], pct);
    //    outPoses[0] = interp.ToMatrix();
    //}
    //else
    //{
    //    outPoses[0] = Matrix4::Identity;
    //}

    //const std::vector<Skeleton::Bone>& bones = mSkinMesh->GetBones();
    //// Now setup the poses for the rest
    //for (size_t bone = 1; bone < numBones; bone++)
    //{
    //    Matrix4 localMat; // (Defaults to identity)
    //    if (mTracks[bone].size() > 0)
    //    {
    //        BoneTransform interp = BoneTransform::Interpolate(mTracks[bone][frame],
    //            mTracks[bone][nextFrame], pct);
    //        localMat = interp.ToMatrix();
    //    }

    //    outPoses[bone] = localMat * outPoses[bones[bone].mParent];
    //}
}

//void Animation::Update(float timeInSeconds)
//{
//    float TicksPerSecond = (float)(m_pScene->mAnimations[mAnimIndex]->mTicksPerSecond != NULL ? m_pScene->mAnimations[mAnimIndex]->mTicksPerSecond : 25.0f);
//    float TimeInTicks = timeInSeconds * TicksPerSecond;
//    float Duration = 0.0f;  // AnimationのDurationの整数部分が入る
//    float fraction = modf((float)m_pScene->mAnimations[mAnimIndex]->mDuration, &Duration);
//    float AnimationTimeTicks = fmod(TimeInTicks, Duration);
//
//}
