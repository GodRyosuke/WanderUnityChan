#include "Actor.hpp"
#include "Game.hpp"
#include "Component.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtc/type_ptr.hpp"
#include "gtx/rotate_vector.hpp"
#include "gtx/vector_angle.hpp"

Actor::Actor(Game* game)
    : mPosition(glm::vec3(0.f))
    , mRotation(glm::mat4(1.f))
    , mScale(1.0f)
    , mGame(game)
    , mRecomputeWorldTransform(true)
{
    mGame->AddActor(this);
}

Actor::~Actor()
{
    mGame->RemoveActor(this);
    // Need to delete components
    // Because ~Component calls RemoveComponent, need a different style loop
    while (!mComponents.empty())
    {
        delete mComponents.back();
    }
}

void Actor::Update(float deltaTime)
{
    ComputeWorldTransform();

    UpdateComponents(deltaTime);
    UpdateActor(deltaTime);

    ComputeWorldTransform();
}

void Actor::UpdateComponents(float deltaTime)
{
    for (auto comp : mComponents)
    {
        comp->Update(deltaTime);
    }
}

void Actor::UpdateActor(float deltaTime)
{
}

void Actor::ProcessInput(const uint8_t* keyState)
{
    for (auto comp : mComponents)
    {
        comp->ProcessInput(keyState);
    }

    ActorInput(keyState);
}

void Actor::ActorInput(const uint8_t* keyState)
{

}

glm::vec3 Actor::GetForward() const
{
    glm::vec3 trans = mRotation * glm::vec4(1.f, 0.f, 0.f, 1.f);
    return trans;
}

glm::vec3 Actor::GetRight() const
{
    glm::vec3 trans = mRotation * glm::vec4(0.f, 1.f, 0.f, 1.f);
    return trans;
}


void Actor::RotateToNewForward(const glm::vec3& forward)
{
    // Figure out difference between original (unit x) and new
    float dot = glm::dot(glm::vec3(1.f, 0.f, 0.f), forward);
    float angle = acos(dot);
    // Facing down X
    if (dot > 0.9999f)
    {
        SetRotation(glm::mat4(1.f));
    }
    // Facing down -X
    else if (dot < -0.9999f)
    {
        SetRotation(glm::rotate(glm::mat4(1.f), (float)M_PI, glm::vec3(0.f, 0.f, 1.f)));
    }
    else
    {
        // Rotate about axis from cross product
        glm::vec3 axis = glm::cross(glm::vec3(1.f, 0.f, 0.f), forward);
        glm::normalize(axis);
        SetRotation(glm::rotate(glm::mat4(1.f), angle, axis));
    }
}

void Actor::ComputeWorldTransform()
{
    if (mRecomputeWorldTransform)
    {
        //mRecomputeWorldTransform = false;
        // Scale, then rotate, then translate
        glm::mat4 ScaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(mScale, mScale, mScale));
        glm::mat4 TranslateMat = glm::translate(glm::mat4(1.0f), mPosition);
        mWorldTransform = TranslateMat * mRotation * ScaleMat;

        // Inform components world transform updated
        for (auto comp : mComponents)
        {
            comp->OnUpdateWorldTransform();
        }
    }
}

void Actor::AddComponent(Component* component)
{
    // Find the insertion point in the sorted vector
    // (The first element with a order higher than me)
    int myOrder = component->GetUpdateOrder();
    auto iter = mComponents.begin();
    for (;
        iter != mComponents.end();
        ++iter)
    {
        if (myOrder < (*iter)->GetUpdateOrder())
        {
            break;
        }
    }

    // Inserts element before position of iterator
    mComponents.insert(iter, component);
}

void Actor::RemoveComponent(Component* component)
{
    auto iter = std::find(mComponents.begin(), mComponents.end(), component);
    if (iter != mComponents.end())
    {
        mComponents.erase(iter);
    }
}
