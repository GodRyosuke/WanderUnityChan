#pragma once

#include <vector>

class FBXNodeMesh {
public:
	FBXNodeMesh();

private:
	unsigned int mNumChilds;
	std::vector<FBXNodeMesh*> mChilds;
};