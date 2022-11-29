#pragma once

#include "Actor.hpp"

class ActorUnityChan : public Actor {
public:
    ActorUnityChan(class Game* game);

private:
    class MeshComponent* mMeshComp;
};
