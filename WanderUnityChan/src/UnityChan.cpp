#define _USE_MATH_DEFINES
#include "UnityChan.hpp"
#include "deFBXMesh.hpp"
#include "Shader.hpp"
#include "gtc/matrix_transform.hpp"
#include "Mesh.hpp"
#include "FBXLoader/Mesh.hpp"

UnityChan::UnityChan()
{
	mMesh = new deFBXMesh(true);
	mMesh->Load("UnityChan");

	mPos = glm::vec3(2.f, 2.f, 0.f);
	mRotate = glm::rotate(glm::mat4(1.0f), (float)M_PI / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	mScale = 1.f;

	mFBXMesh = new FBXMesh();
	if (!mFBXMesh->Load("UnityChan")) {
		printf("error: failed to load UnityChan\n");
		exit(-1);
	}

	//mScale = 0.01f;

	//mAssimpMesh = new Mesh();
	//mAssimpMesh->SetMeshPos(mPos);
	//mAssimpMesh->SetMeshRotate(mRotate);
	//mAssimpMesh->SetMeshScale(mScale);
	//mAssimpMesh->Load("./resources/UnityChan/", "UnityChan.fbx");
}

void UnityChan::Update(float deltatime)
{
	glm::mat4 ScaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(mScale, mScale, mScale));
	glm::mat4 TranslateMat = glm::translate(glm::mat4(1.0f), mPos);
	mWorldTransform = TranslateMat * mRotate * ScaleMat;
}

void UnityChan::Draw(Shader* shader)
{
	shader->UseProgram();
	shader->SetMatrixUniform("ModelTransform", mWorldTransform);
	mFBXMesh->Draw(shader);
 	//mMesh->BindVertexArray();
	//mMesh->Draw(shader);

	//mMesh->DrawArray();
	//mMesh->UnBindVertexArray();

	//mAssimpMesh->Draw(shader, 1000.0f);
}



//
//bool UnityChan::Load(std::string FileRoot, std::string MeshFile, std::vector<std::string>AnimationFiles)
//{
//	// Meshの頂点データなどを読みだす
//	if (!SkinMesh::Load(FileRoot, MeshFile)) {
//		printf("error: failed to load skin mesh file\n");
//		return false;
//	}
//	// Animation読み出し
//	for (auto animFile : AnimationFiles) {
//		std::string filePath = FileRoot + animFile;
//		AnimationData* animData = new AnimationData(filePath);
//		animData->innerAnimIndex = 3;
//		mAnimationData.push_back(animData);
//	}
//
//
//	return true;
//}
//
//const aiAnimation* UnityChan::SetAnimPointer()
//{
//	int innerAnimIndex = mAnimationData[mAnimIndex]->innerAnimIndex;
//	return mAnimationData[mAnimIndex]->animScene->mAnimations[innerAnimIndex];
//}
//
//void UnityChan::UpdateTransform(Shader* shader, float timeInSeconds)
//{
//	Mesh::UpdateTransform(shader, timeInSeconds);
//
//
//	// 現在時刻のBone Transformを取得
//	std::vector<glm::mat4> BoneMatrixPalete;
//	const aiScene* animScene = mAnimationData[mAnimIndex]->animScene;
//	int innerAnimIndex = mAnimationData[mAnimIndex]->innerAnimIndex;
//
//	int num = animScene->mNumAnimations;
//	if (num == 0) {
//		BoneMatrixPalete.resize(m_BoneInfo.size());
//		for (unsigned int i = 0; i < m_BoneInfo.size(); i++) {
//			BoneMatrixPalete[i] = glm::mat4(1.0f);
//		}
//		return;
//	}
//
//	float TicksPerSecond = (float)(animScene->mAnimations[innerAnimIndex]->mTicksPerSecond != NULL ?
//		animScene->mAnimations[innerAnimIndex]->mTicksPerSecond : 25.0f);
//	float TimeInTicks = timeInSeconds * TicksPerSecond;
//	float Duration = 0.0f;  // AnimationのDurationの整数部分が入る
//	float fraction = modf((float)animScene->mAnimations[innerAnimIndex]->mDuration, &Duration);
//	float AnimationTimeTicks = fmod(TimeInTicks, Duration);
//
//
//	glm::mat4 Identity = glm::mat4(1);
//	// Nodeの階層構造にしたがって、AnimationTicks時刻における各BoneのTransformを求める
//	ReadNodeHierarchy(AnimationTimeTicks, animScene->mRootNode, Identity);
//	BoneMatrixPalete.resize(m_BoneInfo.size());
//
//	for (unsigned int i = 0; i < m_BoneInfo.size(); i++) {
//		BoneMatrixPalete[i] = m_BoneInfo[i].FinalTransformation;
//	}
//
//
//	// Shaderに渡す
//	for (int i = 0; i < BoneMatrixPalete.size(); i++) {
//		std::string uniformName = "uMatrixPalette[" + std::to_string(i) + ']';
//		shader->SetMatrixUniform(uniformName, BoneMatrixPalete[i]);
//	}
//}


//void UnityChan::Draw(Shader* shader, float timeInSeconds, int animIdx)
//{
//
//}