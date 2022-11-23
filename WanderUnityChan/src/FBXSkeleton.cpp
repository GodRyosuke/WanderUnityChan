#include "FBXSkeleton.hpp"
#include "glm.hpp"
#include <assert.h>


namespace {
    FbxAMatrix GetGeometry(FbxNode* pNode)
    {
        const FbxVector4 lT = pNode->GetGeometricTranslation(FbxNode::eSourcePivot);
        const FbxVector4 lR = pNode->GetGeometricRotation(FbxNode::eSourcePivot);
        const FbxVector4 lS = pNode->GetGeometricScaling(FbxNode::eSourcePivot);

        return FbxAMatrix(lT, lR, lS);
    }


    // Get the matrix of the given pose
    FbxAMatrix GetPoseMatrix(FbxPose* pPose, int pNodeIndex)
    {
        FbxAMatrix lPoseMatrix;
        FbxMatrix lMatrix = pPose->GetMatrix(pNodeIndex);

        memcpy((double*)lPoseMatrix, (double*)lMatrix, sizeof(lMatrix.mData));

        return lPoseMatrix;
    }

    FbxAMatrix GetGlobalPosition(FbxNode* pNode, const FbxTime& pTime, FbxPose* pPose, FbxAMatrix* pParentGlobalPosition)
    {
        FbxAMatrix lGlobalPosition;
        bool        lPositionFound = false;

        if (pPose)
        {
            int lNodeIndex = pPose->Find(pNode);

            if (lNodeIndex > -1)
            {
                // The bind pose is always a global matrix.
                // If we have a rest pose, we need to check if it is
                // stored in global or local space.
                if (pPose->IsBindPose() || !pPose->IsLocalMatrix(lNodeIndex))
                {
                    lGlobalPosition = GetPoseMatrix(pPose, lNodeIndex);
                }
                else
                {
                    // We have a local matrix, we need to convert it to
                    // a global space matrix.
                    FbxAMatrix lParentGlobalPosition;

                    if (pParentGlobalPosition)
                    {
                        lParentGlobalPosition = *pParentGlobalPosition;
                    }
                    else
                    {
                        if (pNode->GetParent())
                        {
                            lParentGlobalPosition = GetGlobalPosition(pNode->GetParent(), pTime, pPose, NULL);
                        }
                    }

                    FbxAMatrix lLocalPosition = GetPoseMatrix(pPose, lNodeIndex);
                    lGlobalPosition = lParentGlobalPosition * lLocalPosition;
                }

                lPositionFound = true;
            }
        }

        if (!lPositionFound)
        {
            // There is no pose entry for that node, get the current global position instead.

            // Ideally this would use parent global position and local position to compute the global position.
            // Unfortunately the equation 
            //    lGlobalPosition = pParentGlobalPosition * lLocalPosition
            // does not hold when inheritance type is other than "Parent" (RSrs).
            // To compute the parent rotation and scaling is tricky in the RrSs and Rrs cases.
            lGlobalPosition = pNode->EvaluateGlobalTransform(pTime);
        }

        return lGlobalPosition;
    }


    void ComputeShapeDeformation(FbxMesh* pMesh, FbxTime& pTime, FbxAnimLayer* pAnimLayer, FbxVector4* pVertexArray)
    {
        int lVertexCount = pMesh->GetControlPointsCount();

        FbxVector4* lSrcVertexArray = pVertexArray;
        FbxVector4* lDstVertexArray = new FbxVector4[lVertexCount];
        memcpy(lDstVertexArray, pVertexArray, lVertexCount * sizeof(FbxVector4));

        int lBlendShapeDeformerCount = pMesh->GetDeformerCount(FbxDeformer::eBlendShape);
        for (int lBlendShapeIndex = 0; lBlendShapeIndex < lBlendShapeDeformerCount; ++lBlendShapeIndex)
        {
            FbxBlendShape* lBlendShape = (FbxBlendShape*)pMesh->GetDeformer(lBlendShapeIndex, FbxDeformer::eBlendShape);

            int lBlendShapeChannelCount = lBlendShape->GetBlendShapeChannelCount();
            for (int lChannelIndex = 0; lChannelIndex < lBlendShapeChannelCount; ++lChannelIndex)
            {
                FbxBlendShapeChannel* lChannel = lBlendShape->GetBlendShapeChannel(lChannelIndex);
                if (lChannel)
                {
                    // Get the percentage of influence on this channel.
                    FbxAnimCurve* lFCurve = pMesh->GetShapeChannel(lBlendShapeIndex, lChannelIndex, pAnimLayer);
                    if (!lFCurve) continue;
                    double lWeight = lFCurve->Evaluate(pTime);


                    int lShapeCount = lChannel->GetTargetShapeCount();
                    double* lFullWeights = lChannel->GetTargetShapeFullWeights();

                    // Find out which scope the lWeight falls in.
                    int lStartIndex = -1;
                    int lEndIndex = -1;
                    for (int lShapeIndex = 0; lShapeIndex < lShapeCount; ++lShapeIndex)
                    {
                        if (lWeight > 0 && lWeight <= lFullWeights[0])
                        {
                            lEndIndex = 0;
                            break;
                        }
                        if (lWeight > lFullWeights[lShapeIndex] && lWeight < lFullWeights[lShapeIndex + 1])
                        {
                            lStartIndex = lShapeIndex;
                            lEndIndex = lShapeIndex + 1;
                            break;
                        }
                    }

                    FbxShape* lStartShape = NULL;
                    FbxShape* lEndShape = NULL;
                    if (lStartIndex > -1)
                    {
                        lStartShape = lChannel->GetTargetShape(lStartIndex);
                    }
                    if (lEndIndex > -1)
                    {
                        lEndShape = lChannel->GetTargetShape(lEndIndex);
                    }

                    //The weight percentage falls between base geometry and the first target shape.
                    if (lStartIndex == -1 && lEndShape)
                    {
                        double lEndWeight = lFullWeights[0];
                        // Calculate the real weight.
                        lWeight = (lWeight / lEndWeight) * 100;
                        // Initialize the lDstVertexArray with vertex of base geometry.
                        memcpy(lDstVertexArray, lSrcVertexArray, lVertexCount * sizeof(FbxVector4));
                        for (int j = 0; j < lVertexCount; j++)
                        {
                            // Add the influence of the shape vertex to the mesh vertex.
                            FbxVector4 lInfluence = (lEndShape->GetControlPoints()[j] - lSrcVertexArray[j]) * lWeight * 0.01;
                            lDstVertexArray[j] += lInfluence;
                        }
                    }
                    //The weight percentage falls between two target shapes.
                    else if (lStartShape && lEndShape)
                    {
                        double lStartWeight = lFullWeights[lStartIndex];
                        double lEndWeight = lFullWeights[lEndIndex];
                        // Calculate the real weight.
                        lWeight = ((lWeight - lStartWeight) / (lEndWeight - lStartWeight)) * 100;
                        // Initialize the lDstVertexArray with vertex of the previous target shape geometry.
                        memcpy(lDstVertexArray, lStartShape->GetControlPoints(), lVertexCount * sizeof(FbxVector4));
                        for (int j = 0; j < lVertexCount; j++)
                        {
                            // Add the influence of the shape vertex to the previous shape vertex.
                            FbxVector4 lInfluence = (lEndShape->GetControlPoints()[j] - lStartShape->GetControlPoints()[j]) * lWeight * 0.01;
                            lDstVertexArray[j] += lInfluence;
                        }
                    }
                }//If lChannel is valid
            }//For each blend shape channel
        }//For each blend shape deformer

        memcpy(pVertexArray, lDstVertexArray, lVertexCount * sizeof(FbxVector4));

        delete[] lDstVertexArray;
    }

}

FBXSkeleton::FBXSkeleton()
	:mBoneIndices(0)
	,mBoneWeights(0)
{

}

bool FBXSkeleton::Load(FbxMesh* mesh)
{
    const bool lHasShape = mesh->GetShapeCount() > 0;
    if (lHasShape)
    {
        // Deform the vertex array with the shapes.
        // おそらく各時刻のBoneMatrixの補完処理？
        //ComputeShapeDeformation(mesh, pTime, pAnimLayer, lVertexArray);
    }


	int lSkinCount = mesh->GetDeformerCount(FbxDeformer::eSkin);
	if (lSkinCount == 0) {
		printf("this mesh doesnt have skin\n");
		return true;
	}

    // Deform the vertex array with the skin deformer.
    FbxSkin* lSkinDeformer = (FbxSkin*)mesh->GetDeformer(0, FbxDeformer::eSkin);
    FbxSkin::EType lSkinningType = lSkinDeformer->GetSkinningType();
    assert((lSkinningType == FbxSkin::eLinear) || (lSkinningType == FbxSkin::eRigid));

    // Linear Deformation
	FbxCluster::ELinkMode lClusterMode = ((FbxSkin*)mesh->GetDeformer(0, FbxDeformer::eSkin))->GetCluster(0)->GetLinkMode();

    assert(lClusterMode != FbxCluster::eAdditive);

	int lVertexCount = mesh->GetControlPointsCount();
	//glm::vec4* controlPointNodeIndices = new glm::vec4[lVertexCount];
	mBoneIndices.resize(lVertexCount);
	mBoneWeights.resize(lVertexCount);
    deBoneMatrixPallete.resize(lVertexCount);
    std::vector<VertexBoneData> deBones(lVertexCount);
    std::vector<unsigned int> deBoneIdxArray(lVertexCount);
    std::vector<float> deBoneWeightArray(lVertexCount);

    mBoneWeightSchalar.resize(lVertexCount);
    for (int i = 0; i < lVertexCount; i++) {
        mBoneWeightSchalar[i] = 0.f;
        deBoneMatrixPallete[i] = glm::mat4(1.0f);
        deBoneWeightArray[i] = 0.f;
    }

	const int lPolygonCount = mesh->GetPolygonCount();

	printf("control points count: %d, pol * 3: %d\n", lVertexCount, lPolygonCount * 3);
	FbxAMatrix* lClusterDeformation = new FbxAMatrix[lVertexCount];
	memset(lClusterDeformation, 0, lVertexCount * (unsigned int)sizeof(FbxAMatrix));

	double* lClusterWeight = new double[lVertexCount];
	memset(lClusterWeight, 0, lVertexCount * sizeof(double));
    FbxAMatrix pGlobalPosition;

	//if (lClusterMode == FbxCluster::eAdditive)
	//{
	//	for (int i = 0; i < lVertexCount; ++i)
	//	{
	//		lClusterDeformation[i].SetIdentity();
	//	}
	//}


	// For all skins and all clusters, accumulate their deformation and weight
	// on each vertices and store them in lClusterDeformation and lClusterWeight.
	mBones.resize(mesh->GetPolygonCount() * 3);
	assert(lSkinCount == 1);

	for (int lSkinIndex = 0; lSkinIndex < lSkinCount; ++lSkinIndex)
	{
		FbxSkin* lSkinDeformer = (FbxSkin*)mesh->GetDeformer(lSkinIndex, FbxDeformer::eSkin);

		int lClusterCount = lSkinDeformer->GetClusterCount();
		for (int lClusterIndex = 0; lClusterIndex < lClusterCount; ++lClusterIndex)
		{
			FbxCluster* lCluster = lSkinDeformer->GetCluster(lClusterIndex);
			if (!lCluster->GetLink())
				continue;

			FbxAMatrix lVertexTransformMatrix;
			lCluster->GetTransformMatrix(lVertexTransformMatrix);
            FbxPose* pose = nullptr;
            pose = mesh->GetScene()->GetPose(10);
            if (pose->IsBindPose()) {
                int x = 0;
            }
            FbxTime pTime = 0;

			//ComputeClusterDeformation(pGlobalPosition, mesh, lCluster, lVertexTransformMatrix, pTime, pPose);
            {
                // とりあえず，Bind PoseのMatrixを取得
                FbxAMatrix lReferenceGlobalInitPosition;
                FbxAMatrix lReferenceGlobalCurrentPosition;
                FbxAMatrix lAssociateGlobalInitPosition;
                FbxAMatrix lAssociateGlobalCurrentPosition;
                FbxAMatrix lClusterGlobalInitPosition;
                FbxAMatrix lClusterGlobalCurrentPosition;

                FbxAMatrix lReferenceGeometry;
                FbxAMatrix lAssociateGeometry;
                FbxAMatrix lClusterGeometry;

                FbxAMatrix lClusterRelativeInitPosition;
                FbxAMatrix lClusterRelativeCurrentPositionInverse;

                lCluster->GetTransformMatrix(lReferenceGlobalInitPosition);
                lReferenceGlobalCurrentPosition = pGlobalPosition;
                // Multiply lReferenceGlobalInitPosition by Geometric Transformation
                lReferenceGeometry = GetGeometry(mesh->GetNode());
                lReferenceGlobalInitPosition *= lReferenceGeometry;

                // Get the link initial global position and the link current global position.
                lCluster->GetTransformLinkMatrix(lClusterGlobalInitPosition);
                lClusterGlobalCurrentPosition = GetGlobalPosition(lCluster->GetLink(), pTime, nullptr, nullptr);

                // Compute the initial position of the link relative to the reference.
                lClusterRelativeInitPosition = lClusterGlobalInitPosition.Inverse() * lReferenceGlobalInitPosition;

                // Compute the current position of the link relative to the reference.
                lClusterRelativeCurrentPositionInverse = lReferenceGlobalCurrentPosition.Inverse() * lClusterGlobalCurrentPosition;

                // Compute the shift of the link relative to the reference.
                lVertexTransformMatrix = lClusterRelativeCurrentPositionInverse * lClusterRelativeInitPosition;
            }

			int lVertexIndexCount = lCluster->GetControlPointIndicesCount();
			for (int k = 0; k < lVertexIndexCount; ++k)
			{
				int vertexIdx = lCluster->GetControlPointIndices()[k];
                assert(vertexIdx < lVertexCount);
                deBoneIdxArray[vertexIdx] = lClusterIndex;


				double lWeight = lCluster->GetControlPointWeights()[k];
                deBones[vertexIdx].AddBoneData(lClusterIndex, lWeight);

				if (lWeight == 0.0)
				{
					continue;
				}

                deBoneWeightArray[vertexIdx] = lWeight;

				for (int idx = 0; idx < 4; idx++) {
					// もしまだこの頂点に登録されていないBoneなら
					if (!mBoneWeights[vertexIdx][idx]) {
						mBoneIndices[vertexIdx][idx] = lClusterIndex; // ???
						mBoneWeights[vertexIdx][idx] = lWeight;
						break;
					}
				}

				//mBones[lIndex].AddBoneData(lClusterCount * lClusterIndex + k, lWeight);

				//printf("\tindex: %d weight: %f\n", lIndex, lWeight);

				// Compute the influence of the link on the vertex.
				// おそらくこれがMatrixPalleteであろう
				FbxAMatrix lInfluence = lVertexTransformMatrix;
                glm::mat4 boneMat = CopyFbxAMat(lVertexTransformMatrix);
                if (lWeight != 1.f) {
                    int x = 0;
                }
                boneMat *= lWeight;
				//MatrixScale(lInfluence, lWeight);
                deBoneMatrixPallete[vertexIdx] += boneMat;
                //MatrixAdd(lClusterDeformation[lIndex], lInfluence);


                mBoneWeightSchalar[vertexIdx] += lWeight;
                lClusterWeight[vertexIdx] += lWeight;


                // Add to the sum of the deformations on the vertex.
                // Add to the sum of weights to either normalize or complete the vertex.

			}//For each vertex			
		}//lClusterCount
	}

	//Actually deform each vertices here by information stored in lClusterDeformation and lClusterWeight
	// ここで各頂点にweightを考慮してmatrixが計算される？
	// vertex ahaderでの処理

    FbxVector4* lVertexArray = NULL;
    lVertexArray = new FbxVector4[lVertexCount];
    memcpy(lVertexArray, mesh->GetControlPoints(), lVertexCount * sizeof(FbxVector4));


    for (int i = 0; i < lVertexCount; i++) {
        if (lClusterMode == FbxCluster::eNormalize) {
            //double lWeight = lClusterWeight[i];
            double lWeight = mBoneWeightSchalar[i];
            if (lWeight != 0.f) {
                glm::mat4 weightMat(1.f);
                weightMat /= lWeight;   
                deBoneMatrixPallete[i] = weightMat * deBoneMatrixPallete[i];
            }
        }
        glm::vec4 vec = CopyFbxVec(lVertexArray[i]);
        glm::vec4 mVec = deBoneMatrixPallete[i] * vec;
    }

    // Control PointsとPolygon, Verticeとの対応付け
    lVertexCount = lPolygonCount * 3;
    float* lVertices = new float[lVertexCount * 4];
    mBoneMatrixPallete.resize(lPolygonCount * 3);
    std::vector<unsigned int> BoneIdxArray(lPolygonCount * 3);

    lVertexCount = 0;
    for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; ++lPolygonIndex)
    {
        for (int lVerticeIndex = 0; lVerticeIndex < 3; ++lVerticeIndex)
        {
            const int lControlPointIndex = mesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex);
            // If the lControlPointIndex is -1, we probably have a corrupted mesh data. At this point,
            // it is not guaranteed that the cache will work as expected.
            assert(lControlPointIndex >= 0);
            lVertices[lVertexCount * 4]     = static_cast<float>(lVertexArray[lControlPointIndex][0]);
            lVertices[lVertexCount * 4 + 1] = static_cast<float>(lVertexArray[lControlPointIndex][1]);
            lVertices[lVertexCount * 4 + 2] = static_cast<float>(lVertexArray[lControlPointIndex][2]);
            lVertices[lVertexCount * 4 + 3] = 1;
            mBoneMatrixPallete[lPolygonIndex * 3 + lVerticeIndex] = deBoneMatrixPallete[lControlPointIndex];
            BoneIdxArray[lPolygonIndex * 3 + lVerticeIndex] = lControlPointIndex;



            mBones[lPolygonIndex * 3 + lVerticeIndex].AddBoneData(
                deBoneIdxArray[lControlPointIndex],
                deBoneWeightArray[lControlPointIndex]
            );

            // 実際に使う
            glm::mat4 mat = deBoneMatrixPallete[BoneIdxArray[lPolygonIndex * 3 + lVerticeIndex]];
            ++lVertexCount;
        }
    }


	//for (int i = 0; i < lVertexCount; i++)
	//{
	//	FbxVector4 lSrcVertex = pVertexArray[i];
	//	FbxVector4& lDstVertex = pVertexArray[i];
	//	double lWeight = lClusterWeight[i];

	//	// Deform the vertex if there was at least a link with an influence on the vertex,
	//	if (lWeight != 0.0)
	//	{
	//		lDstVertex = lClusterDeformation[i].MultT(lSrcVertex);
	//		if (lClusterMode == FbxCluster::eNormalize)
	//		{
	//			// In the normalized link mode, a vertex is always totally influenced by the links. 
	//			lDstVertex /= lWeight;
	//		}
	//		else if (lClusterMode == FbxCluster::eTotalOne)
	//		{
	//			// In the total 1 link mode, a vertex can be partially influenced by the links. 
	//			lSrcVertex *= (1.0 - lWeight);
	//			lDstVertex += lSrcVertex;
	//		}
	//	}
	//}

	delete[] lClusterDeformation;
	delete[] lClusterWeight;


	return true;
}

void FBXSkeleton::Update(float deltaTime)
{
  
}
