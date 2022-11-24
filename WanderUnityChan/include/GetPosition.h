#pragma once

#include <fbxsdk.h>

FbxAMatrix GetGlobalPosition(FbxNode* pNode,
    const FbxTime& pTime,
    FbxPose* pPose = NULL,
    FbxAMatrix* pParentGlobalPosition = NULL);
FbxAMatrix GetPoseMatrix(FbxPose* pPose,
    int pNodeIndex);
FbxAMatrix GetGeometry(FbxNode* pNode);
