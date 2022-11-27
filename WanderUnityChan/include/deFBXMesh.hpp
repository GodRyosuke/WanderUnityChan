#pragma once


#include "fbxsdk.h"
#include "glm.hpp"
#include <vector>
#include <map>
#include "json.hpp"

namespace nl = nlohmann;

class deFBXMesh {
public:


	deFBXMesh(class UnityChan* owner, bool isDrawArray = false, bool isSkeletal = false);
	~deFBXMesh();
	bool Load(std::string folderPath, std::string fileName);
	void Draw(class Shader* shader);
    void Update(float deltaTime);
	void BindVertexArray();
	void UnBindVertexArray();
	void DrawArray();

	void BindTexture(std::string MaterialName);
	void UnBindTexture(std::string materialName);

    //void SetBoneMatrix(std::string name, glm::mat4 matrix)
    //{
    //    mMatrixUniforms.emplace(name, matrix);
    //}
    void SetGlobalBoneTransform(std::string name, glm::mat4 globaltrans);
    void SetLocalBoneTransform(std::string name, glm::mat4 globaltrans);
    void SetOffsetBoneTransform(std::string name, glm::mat4 globaltrans);
    FbxScene* GetScene() const { return mScene; }
    glm::mat4 GetBoneMatrix(std::string name)
    {
        glm::mat4 offsetMat = mMatrixUniforms[name].OffsetMatrix;
        glm::mat4 globalTrans = mMatrixUniforms[name].GlobalTrans;
        return globalTrans * glm::inverse(offsetMat);
    }
	bool GetIsSkinMesh() const { return mIsSkeletal; }
    uint32_t GetCurrentTicks() const{ return mCurrentTicks; }

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

	void LoadMaterial(FbxSurfaceMaterial* material);
	void LoadTexture(FbxTexture* lTexture);

	void LoadNormal(FbxLayerElementNormal* normalElem);
	void LoadUV(FbxLayerElementUV* uvElem);


    struct BoneTransform {
        BoneTransform()
            :GlobalTrans(glm::mat4(1.0f))
            , LocalTrans(glm::mat4(1.f))
            , OffsetMatrix(glm::mat4(1.f))
        {
        }
        glm::mat4 GlobalTrans;
        glm::mat4 LocalTrans;
        glm::mat4 OffsetMatrix;

    };

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
	std::map<std::string, class Texture*> mTextures;

	class NodeMesh* mRootNodeMesh;

	nl::json mMaterialJsonMap;

	unsigned int mVertexArray;
	unsigned int* mVertexBuffers;
	unsigned int mDrawArrayVAO;

    class UnityChan* mUnityChan;
	FbxManager* mManager;
	std::string mMeshFileName;

    uint32_t mCurrentTicks;
	bool mIsDrawArray;
	bool mIsSkeletal;

    FbxScene* mScene;

    std::map<std::string, BoneTransform> mMatrixUniforms;
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
