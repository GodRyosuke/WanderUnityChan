#include "FBXSkeleton.hpp"
#include "glm.hpp"
#include <assert.h>


namespace {
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

    // Get the matrix of the given pose
    FbxAMatrix GetPoseMatrix(FbxPose* pPose, int pNodeIndex)
    {
        FbxAMatrix lPoseMatrix;
        FbxMatrix lMatrix = pPose->GetMatrix(pNodeIndex);

        memcpy((double*)lPoseMatrix, (double*)lMatrix, sizeof(lMatrix.mData));

        return lPoseMatrix;
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
        ComputeShapeDeformation(mesh, pTime, pAnimLayer, lVertexArray);
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

	int lVertexCount = mesh->GetControlPointsCount();
	//glm::vec4* controlPointNodeIndices = new glm::vec4[lVertexCount];
	mBoneIndices.resize(lVertexCount);
	mBoneWeights.resize(lVertexCount);

	const int lPolygonCount = mesh->GetPolygonCount();

	printf("control points count: %d, pol * 3: %d\n", lVertexCount, lPolygonCount * 3);
	FbxAMatrix* lClusterDeformation = new FbxAMatrix[lVertexCount];
	memset(lClusterDeformation, 0, lVertexCount * (unsigned int)sizeof(FbxAMatrix));

	double* lClusterWeight = new double[lVertexCount];
	memset(lClusterWeight, 0, lVertexCount * sizeof(double));

	if (lClusterMode == FbxCluster::eAdditive)
	{
		for (int i = 0; i < lVertexCount; ++i)
		{
			lClusterDeformation[i].SetIdentity();
		}
	}


	// For all skins and all clusters, accumulate their deformation and weight
	// on each vertices and store them in lClusterDeformation and lClusterWeight.
	mBones.resize(mesh->GetPolygonCount() * 3);
	assert(lSkinCount == 1);
	printf("skin clout: %d\n", lSkinCount);

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



			//ComputeClusterDeformation(pGlobalPosition, mesh, lCluster, lVertexTransformMatrix, pTime, pPose);

			int lVertexIndexCount = lCluster->GetControlPointIndicesCount();
			for (int k = 0; k < lVertexIndexCount; ++k)
			{
				int vertexIdx = lCluster->GetControlPointIndices()[k];
				if (vertexIdx >= lVertexCount)
					continue;

				// Sometimes, the mesh can have less points than at the time of the skinning
				// because a smooth operator was active when skinning but has been deactivated during export.

				double lWeight = lCluster->GetControlPointWeights()[k];

				if (lWeight == 0.0)
				{
					continue;
				}

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
				//MatrixScale(lInfluence, lWeight);

				if (lClusterMode == FbxCluster::eAdditive)
				{
					// Multiply with the product of the deformations on the vertex.
					//MatrixAddToDiagonal(lInfluence, 1.0 - lWeight);
					lClusterDeformation[vertexIdx] = lInfluence * lClusterDeformation[vertexIdx];

					// Set the link to 1.0 just to know this vertex is influenced by a link.
					lClusterWeight[vertexIdx] = 1.0;
				}
				else // lLinkMode == FbxCluster::eNormalize || lLinkMode == FbxCluster::eTotalOne
				{
					// Add to the sum of the deformations on the vertex.
					//MatrixAdd(lClusterDeformation[lIndex], lInfluence);

					// Add to the sum of weights to either normalize or complete the vertex.
					lClusterWeight[vertexIdx] += lWeight;
				}
			}//For each vertex			
		}//lClusterCount
	}

	//Actually deform each vertices here by information stored in lClusterDeformation and lClusterWeight
	// ここで各頂点にweightを考慮してmatrixが計算される？
	// vertex ahaderでの処理
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
