#define _USE_MATH_DEFINES
#include "ActorUnityChan.hpp"
#include "gtc/matrix_transform.hpp"
#include "SkinMeshComponent.hpp"
#include "Game.hpp"


ActorUnityChan::ActorUnityChan(class Game* game)
    :Actor(game)
{ 
    SetPosition(glm::vec3(4.f, 2.f, 0.f));
    glm::mat4 rotateMat;
    rotateMat = glm::mat4(1.f);
    rotateMat = glm::rotate(glm::mat4(1.0f), (float)M_PI / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    rotateMat *= glm::rotate(glm::mat4(1.0f), -(float)M_PI / 2.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    SetRotation(rotateMat);
    SetScale(0.01f);


    // Load Unity Chan
    mSkinMeshComp = new SkinMeshComponent(this);
    mSkinMeshComp->SetMesh(game->GetMesh("./resources/UnityChan/UnityChan_fbx7binary.fbx", true));
    mSkinMeshComp->PlayAnimation(game->GetAnimation("./resources/UnityChan/unitychan_RUN00_F.fbx"), 0);
}
