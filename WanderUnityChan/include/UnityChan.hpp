#include "Skinning.hpp"

class UnityChan : public SkinMesh
{
public:
	UnityChan(){}
	~UnityChan(){}

	bool Load(std::string FileRoot, std::string MeshFile, std::vector<std::string>AnimationFiles);
	void Draw(Shader* shader, float timeInSeconds, int animIdx = 0);
private:
	struct AnimationData {
		int NumAnimation;
		aiAnimation** Data;
	};

	std::vector<AnimationData> mAnimationData;
};