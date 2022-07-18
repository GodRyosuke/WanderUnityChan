#include "Skinning.hpp"

class UnityChan : public SkinMesh
{
public:
	UnityChan(){}
	~UnityChan(){}

	bool Load(std::string MeshFile, std::vector<std::string>AnimationFiles);
};