#include "Plane.hpp"
#include "Game.hpp"
#include "MeshCompoinent.hpp"

Plane::Plane(Game* game)
    :Actor(game)
{
    mMeshComp = new MeshComponent(this);
    mMeshComp->SetMesh(game->GetMesh("./resources/world/terrain.fbx"));
}



