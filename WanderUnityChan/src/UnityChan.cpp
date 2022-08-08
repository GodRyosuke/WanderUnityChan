#include "UnityChan.hpp"

UnityChan::UnityChan()
	:SkinMesh()
{

}

bool UnityChan::Load(std::string FileRoot, std::string MeshFile, std::vector<std::string>AnimationFiles)
{
	// Meshの頂点データなどを読みだす
	if (!SkinMesh::Load(FileRoot, MeshFile)) {
		printf("error: failed to load skin mesh file\n");
		return false;
	}
	// Animation読み出し
	for (auto animFile : AnimationFiles) {
		std::string filePath = FileRoot + animFile;
		AnimationData* animData = new AnimationData(filePath);
		animData->innerAnimIndex = 3;
		mAnimationData.push_back(animData);
	}


	return true;
}

const aiAnimation* UnityChan::SetAnimPointer()
{
	int innerAnimIndex = mAnimationData[mAnimIndex]->innerAnimIndex;
	return mAnimationData[mAnimIndex]->animScene->mAnimations[innerAnimIndex];
}

void UnityChan::UpdateTransform(Shader* shader, float timeInSeconds)
{
	Mesh::UpdateTransform(shader, timeInSeconds);


	// 現在時刻のBone Transformを取得
	std::vector<glm::mat4> BoneMatrixPalete;
	const aiScene* animScene = mAnimationData[mAnimIndex]->animScene;
	int innerAnimIndex = mAnimationData[mAnimIndex]->innerAnimIndex;

	int num = animScene->mNumAnimations;
	if (num == 0) {
		BoneMatrixPalete.resize(m_BoneInfo.size());
		for (unsigned int i = 0; i < m_BoneInfo.size(); i++) {
			BoneMatrixPalete[i] = glm::mat4(1.0f);
		}
		return;
	}

	float TicksPerSecond = (float)(animScene->mAnimations[innerAnimIndex]->mTicksPerSecond != NULL ?
		animScene->mAnimations[innerAnimIndex]->mTicksPerSecond : 25.0f);
	float TimeInTicks = timeInSeconds * TicksPerSecond;
	float Duration = 0.0f;  // AnimationのDurationの整数部分が入る
	float fraction = modf((float)animScene->mAnimations[innerAnimIndex]->mDuration, &Duration);
	float AnimationTimeTicks = fmod(TimeInTicks, Duration);


	glm::mat4 Identity = glm::mat4(1);
	// Nodeの階層構造にしたがって、AnimationTicks時刻における各BoneのTransformを求める
	ReadNodeHierarchy(AnimationTimeTicks, animScene->mRootNode, Identity);
	BoneMatrixPalete.resize(m_BoneInfo.size());

	for (unsigned int i = 0; i < m_BoneInfo.size(); i++) {
		BoneMatrixPalete[i] = m_BoneInfo[i].FinalTransformation;
	}


	// Shaderに渡す
	for (int i = 0; i < BoneMatrixPalete.size(); i++) {
		std::string uniformName = "uMatrixPalette[" + std::to_string(i) + ']';
		shader->SetMatrixUniform(uniformName, BoneMatrixPalete[i]);
	}
}


//void UnityChan::Draw(Shader* shader, float timeInSeconds, int animIdx)
//{
//
//}