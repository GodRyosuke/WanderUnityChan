#include "SkinMeshComponent.hpp"
#include "Actor.hpp"
#include "Shader.hpp"
#include "Skeleton.hpp"
#include "Animation.hpp"
#include "Mesh.hpp"

SkinMeshComponent::SkinMeshComponent(Actor* owner)
    :MeshComponent(owner)
    ,mAnimation(nullptr)
{
    for (int i = 0; i < m)
}

void SkinMeshComponent::SetBoneMatrices(class Shader* shader)
{
    for (int i = 0; i < mMatrixPallete.size(); i++) {
        std::string uniformName = "uMatrixPalette[" + std::to_string(i) + ']';
        shader->SetMatrixUniform(uniformName, mMatrixPallete[i]);
    }
}

void SkinMeshComponent::Update(float deltaTime)
{
    if (mAnimation && mMesh->GetSkeleton())
    {
        mAnimTime += deltaTime * mAnimPlayRate;
        // Wrap around anim time if past duration
        while (mAnimTime > mAnimation->GetDuration(mAnimIdx))
        {
            mAnimTime -= mAnimation->GetDuration(mAnimIdx);
        }

        // Recompute matrix palette
        ComputeMatrixPalette();
    }
}

float SkinMeshComponent::PlayAnimation(const Animation* anim, int animIdx, float playRate)
{
    mAnimation = anim;
    mAnimTime = 0.0f;
    mAnimPlayRate = playRate;
    mAnimIdx = animIdx;

    if (!mAnimation) { return 0.0f; }

    ComputeMatrixPalette();

    return mAnimation->GetDuration(animIdx);
}

void SkinMeshComponent::ComputeMatrixPalette()
{
    //const std::vector<Matrix4>& globalInvBindPoses = mSkeleton->GetGlobalInvBindPoses();
    //std::vector<Matrix4> currentPoses;
    mAnimation->GetGlobalPoseAtTime(mMatrixPallete, mMesh->GetSkeleton(), mAnimTime, mAnimIdx);

    // Setup the palette for each bone
    //for (size_t i = 0; i < mSkeleton->GetNumBones(); i++)
    //{
    //    // Global inverse bind pose matrix times current pose matrix
    //    mPalette.mEntry[i] = globalInvBindPoses[i] * currentPoses[i];
    //}
}


//void SkinMeshComponent::Draw(Shader* shader)
//{
//    if (mMesh)
//    {
//        // Set the world transform
//        shader->SetMatrixUniform("ModelTransform",
//            mOwner->GetWorldTransform());
//        // Set the matrix palette
//        //shader->SetMatrixUniforms("uMatrixPalette", &mPalette.mEntry[0],
//        //    MAX_NUM_BONES_PER_VERTEX);
//        // Set specular power
//        shader->SetFloatUniform("uSpecPower", mMesh->GetSpecPower());
//        // Set the active texture
//        Texture* t = mMesh->GetTexture(mTextureIndex);
//        if (t)
//        {
//            t->SetActive();
//        }
//        // Set the mesh's vertex array as active
//        VertexArray* va = mMesh->GetVertexArray();
//        va->SetActive();
//        // Draw
//        glDrawElements(GL_TRIANGLES, va->GetNumIndices(), GL_UNSIGNED_INT, nullptr);
//    }
//}

