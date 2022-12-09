#pragma once

#include "Component.hpp"

class MeshComponent : public Component {
public:
    MeshComponent(class Actor* owner);
    virtual void Draw(class Shader* shader);
    void Update(float deltatime) override;


    void SetMesh(const class wMesh* mesh) { mMesh = mesh; }
    void SetMesh(const class SkinMesh* mesh) { mSkinMesh = mesh; }
    void SetMesh(const class Animation* anim) { mAnimation = anim; }
    bool GetIsSkeletal();

private:
    const class wMesh* mMesh;
    const class SkinMesh* mSkinMesh;
    const class Animation* mAnimation;
};
