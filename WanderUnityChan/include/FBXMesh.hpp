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
	void DrawArray();

private:
	struct Material {
		std::string Name;
		std::vector<class Texture*> Textures;
	};

	void ShowNodeNames(FbxNode* node, int indent);
	void LoadNode(FbxNode* node);
	void LoadMesh(FbxMesh* mesh);

	void LoadMaterial(FbxSurfaceMaterial* material);

	void LoadNormal(FbxLayerElementNormal* normalElem);
	void LoadUV(FbxLayerElementUV* uvElem);



	void PopulateBuffers();
	void DrawArrayPB();

	std::vector<glm::vec3> mPositions;
	std::vector<glm::vec3> mNormals;
	std::vector<glm::vec2> mTexCoords;
	std::vector<unsigned int> mIndices;
	std::vector<Material> mMaterials;

	unsigned int mVertexArray;
	unsigned int mDrawArrayVAO;

	FbxManager* mManager;
	std::string mMeshFileName;
};