#include "MeshCompoinent.hpp"
#include "Actor.hpp"
#include "Shader.hpp"
#include "Game.hpp"
#include "wMesh.hpp"
#include "VertexArray.hpp"

MeshComponent::MeshComponent(Actor* owner)
    :Component(owner)
    ,mwMesh(nullptr)
    ,mMesh(nullptr)
{
    mOwner->GetGame()->AddMeshComp(this);
}

void MeshComponent::Update(float deltatime)
{

}

void MeshComponent::Draw(Shader* shader)
{
    if (mMesh) {
        const float scale = mOwner->GetScale();
        glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(scale, scale, scale));
        glm::mat4 translateMat = glm::translate(glm::mat4(1.0f), mOwner->GetPosition());
        glm::mat4 transformMat = translateMat * mOwner->GetRotation() * scaleMat;
        shader->SetMatrixUniform("ModelTransform", transformMat);

        VertexArray* vao = mMesh->GetVertexArray();
        vao->SetActive();
        for (int subMeshIdx = 0; subMeshIdx < vao->GetNumSubMeshes(); subMeshIdx++) {
            
        }
    }
}

bool MeshComponent::GetIsSkeletal()
{
    return true;
}

