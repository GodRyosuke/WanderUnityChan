#include "UnityChan.hpp"

bool UnityChan::Load(std::string FileRoot, std::string MeshFile, std::vector<std::string>AnimationFiles)
{
	// Mesh�̒��_�f�[�^�Ȃǂ�ǂ݂���
	if (!SkinMesh::Load(FileRoot, MeshFile)) {
		printf("error: failed to load skin mesh file\n");
		return false;
	}
	// Animation�ǂݏo��
	for (auto animFile : AnimationFiles) {
		std::string filePath = FileRoot + animFile;

		Assimp::Importer pImpoter;
		const aiScene* pScene = pImpoter.ReadFile(filePath.c_str(), ASSIMP_LOAD_FLAGS);

		if (!pScene) {
			printf("Error parsing '%s': '%s'\n", filePath.c_str(), pImpoter.GetErrorString());
			return false;
		}

		AnimationData animData;
		animData.NumAnimation = pScene->mNumAnimations;
		animData.Data = pScene->mAnimations;
		mAnimationData.push_back(animData);
	}


	return true;
}

void UnityChan::Draw(Shader* shader, float timeInSeconds, int animIdx)
{

}