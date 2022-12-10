#include "MeshCompoinent.hpp"
#include "Actor.hpp"
#include "Shader.hpp"
#include "Game.hpp"
#include "wMesh.hpp"
#include "VertexArray.hpp"

MeshComponent::MeshComponent(Actor* owner, bool isSkeletal)
    :Component(owner)
    ,mwMesh(nullptr)
    ,mMesh(nullptr)
    ,mIsSkeletal(isSkeletal)
{
    mOwner->GetGame()->AddMeshComp(this);
}

void MeshComponent::Update(float deltatime)
{

}

void MeshComponent::Draw(Shader* shader)
{
    shader->UseProgram();
    if (mMesh) {
        const float scale = mOwner->GetScale();
        glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(scale, scale, scale));
        glm::mat4 translateMat = glm::translate(glm::mat4(1.0f), mOwner->GetPosition());
        glm::mat4 transformMat = translateMat * mOwner->GetRotation() * scaleMat;
        shader->SetMatrixUniform("ModelTransform", transformMat);

        VertexArray* vao = mMesh->GetVertexArray();
        vao->SetActive();
        for (int subMeshIdx = 0; subMeshIdx < mMesh->GetSubMeshNum(); subMeshIdx++) {
            unsigned int numIndices;
            unsigned int baseVertex;
            unsigned int baseIndex;
            Mesh::Material material;
            mMesh->GetMeshEntry(subMeshIdx, numIndices, baseVertex, baseIndex, material);

            // Material設定
            shader->SetVectorUniform("matAmbientColor", material.AmbientColor);
            //shader->SetVectorUniform("uDirLight.mDirection", glm::vec3(0, -0.707, -0.707));
            shader->SetVectorUniform("matDiffuseColor", material.DiffuseColor);
            //shader->SetVectorUniform("uDirLight.mSpecColor", m_Materials[MaterialIndex].SpecColor);
            shader->SetFloatUniform("matSpecPower", material.SpecPower);
            shader->SetVectorUniform("matSpecColor", material.SpecColor);
            //shader->SetFloatUniform("gMatSpecularIntensity", 1.f);
            //shader->SetFloatUniform("gMatSpecularIntensity", 1.0f);

            if (material.DiffuseTexture) {
                material.DiffuseTexture->BindTexture(GL_TEXTURE0);
            }

            // Mesh描画
            glDrawElementsBaseVertex(GL_TRIANGLES,
                numIndices,
                GL_UNSIGNED_INT,
                (void*)(sizeof(unsigned int) * baseIndex),
                baseVertex);
        }
    }
}

