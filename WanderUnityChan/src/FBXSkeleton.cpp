#include "FBXSkeleton.hpp"

#include <assert.h>

FBXSkeleton::FBXSkeleton()
{

}

bool FBXSkeleton::Load(FbxMesh* mesh)
{
    // Skinning ‚Ìtype’²¸
    const int lSkinCount = mesh->GetDeformerCount(FbxDeformer::eSkin);
    int lClusterCount = 0;
    for (int lSkinIndex = 0; lSkinIndex < lSkinCount; ++lSkinIndex)
    {
        lClusterCount += ((FbxSkin*)(mesh->GetDeformer(lSkinIndex, FbxDeformer::eSkin)))->GetClusterCount();
    }
    if (lClusterCount)
    {
        // Deform the vertex array with the skin deformer.
        FbxSkin* lSkinDeformer = (FbxSkin*)mesh->GetDeformer(0, FbxDeformer::eSkin);
        FbxSkin::EType lSkinningType = lSkinDeformer->GetSkinningType();
        assert((lSkinningType == FbxSkin::eLinear) || (lSkinningType == FbxSkin::eRigid));

        // Linear Deformation
		FbxCluster::ELinkMode lClusterMode = ((FbxSkin*)mesh->GetDeformer(0, FbxDeformer::eSkin))->GetCluster(0)->GetLinkMode();

		int lVertexCount = mesh->GetControlPointsCount();
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
		int lSkinCount = mesh->GetDeformerCount(FbxDeformer::eSkin);
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
				ComputeClusterDeformation(pGlobalPosition, mesh, lCluster, lVertexTransformMatrix, pTime, pPose);

				int lVertexIndexCount = lCluster->GetControlPointIndicesCount();
				for (int k = 0; k < lVertexIndexCount; ++k)
				{
					int lIndex = lCluster->GetControlPointIndices()[k];

					// Sometimes, the mesh can have less points than at the time of the skinning
					// because a smooth operator was active when skinning but has been deactivated during export.
					if (lIndex >= lVertexCount)
						continue;

					double lWeight = lCluster->GetControlPointWeights()[k];

					if (lWeight == 0.0)
					{
						continue;
					}

					// Compute the influence of the link on the vertex.
					FbxAMatrix lInfluence = lVertexTransformMatrix;
					MatrixScale(lInfluence, lWeight);

					if (lClusterMode == FbxCluster::eAdditive)
					{
						// Multiply with the product of the deformations on the vertex.
						MatrixAddToDiagonal(lInfluence, 1.0 - lWeight);
						lClusterDeformation[lIndex] = lInfluence * lClusterDeformation[lIndex];

						// Set the link to 1.0 just to know this vertex is influenced by a link.
						lClusterWeight[lIndex] = 1.0;
					}
					else // lLinkMode == FbxCluster::eNormalize || lLinkMode == FbxCluster::eTotalOne
					{
						// Add to the sum of the deformations on the vertex.
						MatrixAdd(lClusterDeformation[lIndex], lInfluence);

						// Add to the sum of weights to either normalize or complete the vertex.
						lClusterWeight[lIndex] += lWeight;
					}
				}//For each vertex			
			}//lClusterCount
		}

		//Actually deform each vertices here by information stored in lClusterDeformation and lClusterWeight
		for (int i = 0; i < lVertexCount; i++)
		{
			FbxVector4 lSrcVertex = pVertexArray[i];
			FbxVector4& lDstVertex = pVertexArray[i];
			double lWeight = lClusterWeight[i];

			// Deform the vertex if there was at least a link with an influence on the vertex,
			if (lWeight != 0.0)
			{
				lDstVertex = lClusterDeformation[i].MultT(lSrcVertex);
				if (lClusterMode == FbxCluster::eNormalize)
				{
					// In the normalized link mode, a vertex is always totally influenced by the links. 
					lDstVertex /= lWeight;
				}
				else if (lClusterMode == FbxCluster::eTotalOne)
				{
					// In the total 1 link mode, a vertex can be partially influenced by the links. 
					lSrcVertex *= (1.0 - lWeight);
					lDstVertex += lSrcVertex;
				}
			}
		}

		delete[] lClusterDeformation;
		delete[] lClusterWeight;

    }

	return true;
}