#pragma once

#include "Component.hpp"

class MeshComponent : public Component {
public:
    MeshComponent(class Actor* owner);
    virtual void Draw(class Shader* shader);
    void Update(float deltatime) override;

    void SetMesh(const class wMesh* mesh) { mwMesh = mesh; }
    void SetMesh(const class Mesh* mesh) { mMesh = mesh; }
    void SetMesh(const class Animation* anim) { mAnimation = anim; }
    bool GetIsSkeletal();

private:
    const class wMesh* mwMesh;
    const class Mesh* mMesh;
    const class Animation* mAnimation;
};
