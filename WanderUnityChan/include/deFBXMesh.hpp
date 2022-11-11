#pragma once


#include "fbxsdk.h"
#include "glm.hpp"
#include <vector>

class deFBXMesh {
public:
	deFBXMesh(bool isDrawArray = false);
	~deFBXMesh();
	bool Load(std::string fileName);
	void BindVertexArray();
	void UnBindVertexArray();
	void Draw(class Shader* shader);
	void DrawArray();

private:
	struct Material {
		std::string Name;
		unsigned int MaterialIndex;
		std::vector<class Texture*> Textures;
		unsigned int IndexOffset;
		unsigned int TriangleCount;
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
		unsigned int NumIndices;	// このポリゴンの頂点の数
		unsigned int BaseVertex;
		unsigned int BaseIndex;
		unsigned int IndexOffset;
		unsigned int VertexOffset;	// DrawArraysで使う頂点offset
		unsigned int NumVertices;	// 頂点の数
		unsigned int TriangleCount;
	};

	struct dedeMaterialVNT {
		unsigned int VertexArray;
		unsigned int VertexCount;
		unsigned int* VertexBuffers;
		std::vector<glm::vec3>Positions;
		std::vector<glm::vec3>Normals;
		std::vector<glm::vec2>TexCoords;
		std::vector<unsigned int> Indices;
		Material* material;
	};

	void ShowNodeNames(FbxNode* node, int indent);
	void LoadNode(FbxNode* node);
	void LoadMesh(FbxMesh* mesh);
	dedeMaterialVNT* LoadMeshElement(FbxMesh* mesh);
	dedeMaterialVNT* LoadMeshArray(FbxMesh* mesh, unsigned int& vertexOffset);

	Material* LoadMaterial(FbxSurfaceMaterial* material);

	void LoadNormal(FbxLayerElementNormal* normalElem);
	void LoadUV(FbxLayerElementUV* uvElem);



	void PopulateBuffers();
	void DrawArrayPB();

	std::vector<glm::vec3> mPositions;
	std::vector<glm::vec3> mNormals;
	std::vector<glm::vec2> mTexCoords;
	std::vector<unsigned int> mIndices;
	std::vector<Material*> mMaterials;
	struct VNTOffset {
		unsigned int MaterialIndex;
		unsigned int VNTOffset;
	};
	struct deMaterialVNT {
		std::vector<VNTOffset> VNTOffsets;
		std::vector<Material> Materials;
	};
	struct MeshOffset {
		unsigned int VNTOffset;
		Material* material;
	};
	std::vector<MeshOffset> mMeshOffsets;

	std::vector<BasicMeshEntry*> mBasicMeshEntries;
	
	std::vector<dedeMaterialVNT*> mNodeMeshes;
	std::vector<class VAO*> mVAOs;

	class NodeMesh* mRootNodeMesh;

	unsigned int mVertexArray;
	unsigned int* mVertexBuffers;
	unsigned int mDrawArrayVAO;

	FbxManager* mManager;
	std::string mMeshFileName;

	bool mIsDrawArray;
};

class VAO {
public:
	VAO();
	void Bind();
	void CreateVAO();
	void SetVNT(
		std::vector<glm::vec3> positions,
		std::vector<glm::vec3> normals,
		std::vector<glm::vec2> texcoords
	) {
		mPositions = positions; mNormals = normals; mTexCoords = texcoords;
	}
	unsigned int GetVertexCount() { return static_cast<unsigned int>(mPositions.size()); }

private:
	unsigned int mVertexArray;
	unsigned int* mVertexBuffers;
	std::vector<glm::vec3> mPositions;
	std::vector<glm::vec3> mNormals;
	std::vector<glm::vec2> mTexCoords;
};