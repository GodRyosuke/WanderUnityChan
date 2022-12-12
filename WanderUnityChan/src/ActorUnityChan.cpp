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
    // UnityChanの頭の位置
    glm::mat4 meshMat = {
        {0.0157026369f, 0.0416036099f, 0.999011219f, 0.f},
        {0.990548074f, 0.136814535f, 0.00987200066f, 0.f},
        {-0.136268482f, 0.989723265f, -0.0433587097f, 0.f},
        {-128.945908f, -16.3280373f, -1.34681416f, 1.f}
    };
    mSkinMeshComp->PlayAnimation(game->GetAnimation("./resources/UnityChan/unitychan_RUN00_F_fbx7binary.fbx", meshMat), 0);
    //mSkinMeshComp->SetMesh(game->GetMesh("./resources/UnityChan/unitychan2.fbx", true));
    //mSkinMeshComp->PlayAnimation(game->GetAnimation("./resources/UnityChan/unitychan_RUN00_F.fbx"), 0);
    //mSkinMeshComp->SetMesh(game->GetMesh("./resources/SimpleMan/test_output.fbx", true));
    //mSkinMeshComp->PlayAnimation(game->GetAnimation("./resources/SimpleMan/test_output.fbx"), 0);

}
