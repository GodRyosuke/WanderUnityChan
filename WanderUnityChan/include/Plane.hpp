#pragma once

#include "Actor.hpp"

class Plane : public Actor
{
public:
    Plane(class Game* game);

private:
    class MeshComponent* mMeshComp;
};
