#include <glm.hpp>
#include <vector>
#include <fbxsdk.h>
#include <string>
#include "glad/glad.h"
//#include <glew.h>


class NodeMesh {
public:
	NodeMesh(FbxNode* pNode, FbxNode* parentNode, class deFBXMesh* fbxmesh);

	void Draw(class Shader* shader);
    void Update(float deltatime, glm::mat4 = glm::mat4(1.f));
	int GetNumChild() { return mNumChild; }

private:
    enum NodeType {
        MESH
        ,SKELETON
        ,NUM_NODE_TYPE
    };

	struct Material {
		Material() : mTexture(nullptr), AmbientColor(0) {}
		class Texture* mTexture;
		std::vector<std::string> mTextureFileNames;
		glm::vec3 AmbientColor;
		float AmbientFactor;
		glm::vec3 DiffuseColor;
		float DiffuseFactor;
		glm::vec3 SpecColor;
		float SpecFactor;
		float Shinness;
	};

	struct SubMesh
	{
		SubMesh() : IndexOffset(0), TriangleCount(0), Material(nullptr) {}

		int IndexOffset;
		int TriangleCount;
		std::string MaterialName;
		Material* Material;
	};
	enum BUFFER_TYPE {
		INDEX_BUFFER = 0,
		POS_VB = 1,
		TEXCOORD_VB = 2,
		NORMAL_VB = 3,
		NUM_BUFFERS = 4,  // required only for instancing
	};
	bool LoadMesh(FbxMesh* mesh);
	Material* LoadMaterial(FbxSurfaceMaterial* material);
	FbxDouble3 GetMaterialProperty(const FbxSurfaceMaterial* pMaterial,
		const char* pPropertyName,
		const char* pFactorPropertyName,
		std::string& textureName);

	void CreateVAO();
	void CreateVAO(
		const int lPolygonCount,
		unsigned int* lIndices,
		float* lVertices,
		float* lNormals,
		float* lUV,
		bool hasNormal,
		bool hasUV
	);

	int mNumChild;

	GLuint mVertexArray;
	GLuint mVertexBuffers[NUM_BUFFERS];
	GLuint mVertexBuffer;

	bool mIsMesh;

	std::vector<SubMesh*> mSubMeshes;


	std::vector<unsigned int> mIndices;
	std::vector<glm::vec3> mPositions;
	std::vector<glm::vec3> mNormals;
	std::vector<glm::vec2> mTexCoords;

    FbxAMatrix mParentGlobalPositin;

	class deFBXMesh* mOwnerMesh;
	std::string mMeshName;
	class FBXSkeleton* mFBXSkeleton;

    glm::mat4 mGlobalTrans;
    glm::mat4 mLocalTrans;

	std::vector<NodeMesh*> mChilds;

    bool mHasSkin;
    FbxNode* mNode;
    FbxNode* mParentNode;
    NodeType mNodeType;
};
