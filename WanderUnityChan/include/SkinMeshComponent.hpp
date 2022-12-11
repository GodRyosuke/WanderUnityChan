#pragma once

#include "MeshCompoinent.hpp"
#include <vector>
#include "glm.hpp"

class SkinMeshComponent : public MeshComponent
{
public:
    SkinMeshComponent(class Actor* owner);
    //void Draw(class Shader* shader) override;

    void Update(float deltaTime) override;


    // Play an animation. Returns the length of the animation
    float PlayAnimation(const class Animation* anim, int animIdx, float playRate = 1.0f);
private:
    void SetBoneMatrices(class Shader* shader) override;
    void ComputeMatrixPalette();

    std::vector<glm::mat4> mMatrixPallete;
    //const class Skeleton* mSkeleton;
    const class Animation* mAnimation;
    float mAnimPlayRate;
    float mAnimTime;
    float mAnimTicks;
    int mAnimIdx;
};
