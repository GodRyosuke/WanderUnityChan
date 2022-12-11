#pragma once

#include "Component.hpp"

class MeshComponent : public Component {
public:
    MeshComponent(class Actor* owner, bool isSkeletal = false);
    virtual void Draw(class Shader* shader);
    void Update(float deltatime) override;

    void SetMesh(const class wMesh* mesh) { mwMesh = mesh; }
    void SetMesh(const class Mesh* mesh) { mMesh = mesh; }
    const bool GetIsSkeletal() const { return mIsSkeletal; }

protected:
    virtual void SetBoneMatrices(class Shader* shader) {}

    const class wMesh* mwMesh;
    const class Mesh* mMesh;
    bool mIsSkeletal;
};
