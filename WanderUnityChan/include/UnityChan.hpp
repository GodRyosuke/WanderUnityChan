#include "Skinning.hpp"

class UnityChan : public SkinMesh
{
public:
	UnityChan();
	~UnityChan(){}

	void SetAnimIndex(int animIndex) { mAnimIndex = animIndex; }
	bool Load(std::string FileRoot, std::string MeshFile, std::vector<std::string>AnimationFiles);
	//void Draw(Shader* shader, float timeInSeconds, int animIdx = 0);
private:
	struct AnimationData {
		Assimp::Importer animImporter;
		const aiScene* animScene;
		int innerAnimIndex;
		AnimationData(std::string filepath)
		{
			animScene = animImporter.ReadFile(filepath.c_str(), ASSIMP_LOAD_FLAGS);
		}
	};

	virtual void UpdateTransform(Shader* shader, float timeInSeconds) override;
	virtual const aiAnimation* SetAnimPointer() override;


	int mAnimIndex;
	std::vector<AnimationData*> mAnimationData;
};