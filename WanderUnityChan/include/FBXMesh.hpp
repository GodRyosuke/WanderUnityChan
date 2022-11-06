#pragma once


#include "fbxsdk.h"
#include "glm.hpp"
#include <vector>

class FBXMesh {
public:
	FBXMesh(bool isDrawArray = false);
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

	struct BasicMeshEntry {
		BasicMeshEntry()
			:MaterialIndex(0)
			,NumIndices(0)
			,BaseVertex(0)
			,BaseIndex(0)
			,IndexOffset(0)
			,TriangleCount(0)
		{
		}
		unsigned int MaterialIndex;
		unsigned int NumIndices;	// ���̃|���S���̒��_�̐�
		unsigned int BaseVertex;
		unsigned int BaseIndex;
		unsigned int IndexOffset;
		unsigned int VertexOffset;	// DrawArrays�Ŏg�����_offset
		unsigned int NumVertices;	// ���_�̐�
		unsigned int TriangleCount;
	};


	void ShowNodeNames(FbxNode* node, int indent);
	void LoadNode(FbxNode* node);
	void LoadMesh(FbxMesh* mesh);
	bool LoadMeshElement(FbxMesh* mesh);
	bool LoadMeshArray(FbxMesh* mesh);

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
	std::vector<BasicMeshEntry> mBasicMeshEntries;
	
	

	unsigned int mVertexArray;
	unsigned int mDrawArrayVAO;

	FbxManager* mManager;
	std::string mMeshFileName;

	bool mIsDrawArray;
};