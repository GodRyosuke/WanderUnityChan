#define _USE_MATH_DEFINES
#include "UnityChan.hpp"
#include "deFBXMesh.hpp"
#include "Game.hpp"
#include "Shader.hpp"
#include "gtc/matrix_transform.hpp"
#include "Mesh.hpp"
#include "FBXLoader/Mesh.hpp"

UnityChan::UnityChan(Game* game)
	:z_rot(0.f)
    ,mGame(game)
{
	mdeFBXMesh = new deFBXMesh(this, false, true);
	mdeFBXMesh->Load("./resources/UnityChan/", "UnityChan");
    mAnimMesh = new deFBXMesh(this, false, true);
    mAnimMesh->Load("./resources/UnityChan/", "unitychan_RUN00_F");
    //mAnimMesh->Load("./resources/UnityChan/", "unitychan_run00_f");
	//mdeFBXMesh->Load("cacti");
	//mdeFBXMesh->Load("TreasureChest");
	//mdeFBXMesh->Load("TreasureBox2");
	//mdeFBXMesh->Load("Bush_1");

	mPos = glm::vec3(2.f, 2.f, 0.f);
	mRotate = glm::mat4(1.f);
	mRotate = glm::rotate(glm::mat4(1.0f), (float)M_PI / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	mRotate *= glm::rotate(glm::mat4(1.0f), -(float)M_PI / 2.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	mScale = 1.f;

	//mFBXMesh = new FBXMesh();
	//if (!mFBXMesh->Load("UnityChan")) {
	//	printf("error: failed to load UnityChan\n");
	//	exit(-1);
	//}

	//mAssimpMesh = new Mesh();
	//mAssimpMesh->Load("./resources/UnityChan/", "UnityChan.fbx");

	mScale = 0.010f;

	//mAssimpMesh = new Mesh();
	//mAssimpMesh->SetMeshPos(mPos);
	//mAssimpMesh->SetMeshRotate(mRotate);
	//mAssimpMesh->SetMeshScale(mScale);
	//mAssimpMesh->Load("./resources/UnityChan/", "UnityChan.fbx");
}

void UnityChan::Update(float deltatime)
{
	z_rot += 0.01f;
	if (z_rot < 2 * M_PI) {
		z_rot -= 2 * M_PI;
	}
	mRotate = glm::rotate(glm::mat4(1.0f), (float)M_PI / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	mRotate *= glm::rotate(glm::mat4(1.0f), z_rot, glm::vec3(0.0f, 1.0f, 0.0f));

	glm::mat4 ScaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(mScale, mScale, mScale));
	glm::mat4 TranslateMat = glm::translate(glm::mat4(1.0f), mPos);
	mWorldTransform = TranslateMat * mRotate * ScaleMat;

    //mdeFBXMesh->Update(deltatime);
    mAnimMesh->Update(deltatime);
}

void UnityChan::Draw(Shader* shader)
{
	shader->UseProgram();
	shader->SetMatrixUniform("ModelTransform", mWorldTransform);
	//mAssimpMesh->Draw(shader, 0.f);

	//mFBXMesh->Draw(shader);

    // 更新されたアニメーションの情報を，メッシュに流す
    mdeFBXMesh->SetMatrixUniforms(mAnimMesh->GetMatrixUniforms());
	mdeFBXMesh->Draw(shader);
    //mAnimMesh->Draw(shader);
 	//mdeFBXMesh->BindVertexArray();

	////mMesh->DrawArray();
	mdeFBXMesh->UnBindVertexArray();
    

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
