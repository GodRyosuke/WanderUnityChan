#include <fbxsdk.h>

class FBXMesh {
public:
	FBXMesh();
	bool Load(std::string fileName);


private:
	void InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene);

	FbxManager* mSdkManager;
	FbxImporter* mImporter;
	FbxScene* mScene;
};