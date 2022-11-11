#include <glm.hpp>
#include <vector>
#include <fbxsdk.h>
#include <string>
#include "glad/glad.h"
//#include <glew.h>


class NodeMesh {
public:
	NodeMesh(FbxNode* pNode);

	void Draw();
	int GetNumChild() { return mNumChild; }

private:
	struct SubMesh {
		unsigned int TriangleCount;
	};
	enum BUFFER_TYPE {
		INDEX_BUFFER = 0,
		POS_VB = 1,
		TEXCOORD_VB = 2,
		NORMAL_VB = 3,
		NUM_BUFFERS = 4,  // required only for instancing
	};
	bool LoadMesh(FbxMesh* mesh);
	void CreateVAO();

	int mNumChild;

	GLuint mVertexArray;
	//GLuint mVertexBuffers[NUM_BUFFERS];
	GLuint mVertexBuffer;

	bool mIsMesh;

	std::vector<unsigned int> mIndices;
	std::vector<glm::vec3> mPositions;
	std::vector<glm::vec3> mNormals;
	std::vector<glm::vec2> mTexCoords;

	std::vector<NodeMesh*> mChilds;
};