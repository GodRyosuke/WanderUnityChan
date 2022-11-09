#pragma once

#include <fbxsdk.h>

class FBXMesh {
public:
	FBXMesh();
	bool Load(std::string fileName);
	void Draw(class Shader* shader);

private:
	void InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene);
	void LoadCacheRecursive(FbxNode* pNode, FbxAnimLayer* pAnimLayer, bool pSupportVBO);
	void LoadCacheRecursive(FbxScene* pScene, FbxAnimLayer* pAnimLayer, const char* pFbxFileName, bool pSupportVBO);
	void FillPoseArray(FbxScene* pScene, FbxArray<FbxPose*>& pPoseArray);
	void PreparePointCacheData(FbxScene* pScene, FbxTime& pCache_Start, FbxTime& pCache_Stop);
	void DrawNodeRecursive(FbxNode* pNode, FbxAnimLayer* pAnimLayer,
		FbxAMatrix& pParentGlobalPosition, FbxPose* pPose);
	void DrawNode(FbxNode* pNode,
		FbxAnimLayer* pAnimLayer,
		FbxAMatrix& pParentGlobalPosition,
		FbxAMatrix& pGlobalPosition,
		FbxPose* pPose);
	void DrawMesh(FbxNode* pNode, FbxAnimLayer* pAnimLayer,
		FbxAMatrix& pGlobalPosition, FbxPose* pPose);
	void ComputeShapeDeformation(FbxMesh* pMesh, FbxTime& pTime, FbxAnimLayer* pAnimLayer, FbxVector4* pVertexArray);
	void ComputeSkinDeformation(FbxAMatrix& pGlobalPosition,
		FbxMesh* pMesh,
		FbxTime& pTime,
		FbxVector4* pVertexArray,
		FbxPose* pPose);
	void ReadVertexCacheData(FbxMesh* pMesh,
		FbxTime& pTime,
		FbxVector4* pVertexArray);


	FbxArray<FbxString*> mAnimStackNameArray;
	FbxArray<FbxNode*> mCameraArray;
	FbxArray<FbxPose*> mPoseArray;

	FbxManager* mSdkManager;
	FbxImporter* mImporter;
	FbxScene* mScene;
	FbxAnimLayer* mCurrentAnimLayer;

	mutable FbxTime mCache_Start, mCache_Stop;

	bool mSupportVBO;

};