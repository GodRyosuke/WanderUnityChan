#include "MeshCompoinent.hpp"
#include "Actor.hpp"
#include "Shader.hpp"
#include "Game.hpp"
#include "wMesh.hpp"

MeshComponent::MeshComponent(Actor* owner)
    :Component(owner)
    ,mMesh(nullptr)
    ,mSkinMesh(nullptr)
{
    mOwner->GetGame()->AddMeshComp(this);
}

void MeshComponent::Update(float deltatime)
{

}

void MeshComponent::Draw(Shader* shader)
{
    if (mSkinMesh) {

    }
}

bool MeshComponent::GetIsSkeletal()
{
    return true;
}

