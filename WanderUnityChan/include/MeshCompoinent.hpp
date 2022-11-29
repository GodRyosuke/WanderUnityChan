#pragma once

#include "Component.hpp"

class MeshComponent : public Component {
public:
    MeshComponent(class Actor* owner);
    void Draw(class Shader* shader);


    bool GetIsSkeletal();

private:

};
