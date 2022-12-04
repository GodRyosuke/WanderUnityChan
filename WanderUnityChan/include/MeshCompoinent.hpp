#pragma once

#include "Component.hpp"

class MeshComponent : public Component {
public:
    MeshComponent(class Actor* owner);
    virtual void Draw(class Shader* shader);


    void SetMesh(class wMesh* mesh) { mMesh = mesh; }
    bool GetIsSkeletal();

private:
    class wMesh* mMesh;

};
