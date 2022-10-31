#pragma once


#include "fbxsdk.h"
#include "glm.hpp"
#include <vector>

class FBXMesh {
public:
	FBXMesh();
	~FBXMesh();
	bool Load(std::string fileName);
	void BindVertexArray();
	void UnBindVertexArray();
	void Draw(class Shader* shader);

private:
	struct Polygon {
		int Size;
		unsigned int BaseIndex;	// mIndicesÇÃÅAÇ«ÇÃà íuÇÃIndexÇ»ÇÃÇ©ÅH
	};

	void ShowNodeNames(FbxNode* node, int indent);
	void LoadNode(FbxNode* node);
	void LoadMesh(FbxMesh* mesh);

	void LoadNormal(FbxLayerElementNormal* normalElem);
	void LoadUV(FbxLayerElementUV* uvElem);

	void PopulateBuffers();

	std::vector<glm::vec3> mPositions;
	std::vector<glm::vec3> mNormals;
	std::vector<glm::vec2> mTexCoords;
	std::vector<unsigned int> mIndices;

	unsigned int mVertexArray;

	FbxManager* mManager;
};