#include "MeshCompoinent.hpp"
#include "Actor.hpp"
#include "Shader.hpp"
#include "Game.hpp"

MeshComponent::MeshComponent(Actor* owner)
    :Component(owner)
{
    mOwner->GetGame()->AddMeshComp(this);
}

void MeshComponent::Draw(Shader* shader)
{

}

bool MeshComponent::GetIsSkeletal()
{
    return true;
}

