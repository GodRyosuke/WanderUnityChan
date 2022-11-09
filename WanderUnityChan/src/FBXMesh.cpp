#include "FBXLoader/Mesh.hpp"
#include <fbxsdk.h>
#include "glew.h"
#include "Texture.hpp"
#include "FBXLoader/MaterialCache.hpp"
#include "Shader.hpp"
#include "FBXLoader/VBOMesh.hpp"


FBXMesh::FBXMesh()
    :mSdkManager(nullptr)
    ,mImporter(nullptr)
    ,mScene(nullptr)
{}

bool FBXMesh::Load(std::string fileName)
{
    std::string filePath = "./resources/" + fileName + "/" + fileName + ".fbx";

    InitializeSdkObjects(mSdkManager, mScene);

    if (mSdkManager)
    {
        // Create the importer.
        int lFileFormat = -1;
        mImporter = FbxImporter::Create(mSdkManager, "");
        if (!mSdkManager->GetIOPluginRegistry()->DetectReaderFileFormat(filePath.c_str(), lFileFormat))
        {
            // Unrecognizable file format. Try to fall back to FbxImporter::eFBX_BINARY
            lFileFormat = mSdkManager->GetIOPluginRegistry()->FindReaderIDByDescription("FBX binary (*.fbx)");;
        }

        // Initialize the importer by providing a filename.
        if (mImporter->Initialize(filePath.c_str(), lFileFormat) == true)
        {
            printf("Importing file...\n");
        }
        else
        {
            printf("Failed to open file: %s\n", fileName.c_str());
            return false;
        }
    }
    else
    {
        printf("Unable to create the FBX SDK manager\n", fileName.c_str());
        return false;
    }


	bool lResult = false;
	// Make sure that the scene is ready to load.
	lResult = mImporter->Import(mScene);
	if (lResult)
	{
		// Check the scene integrity!
		FbxStatus status;
		FbxArray< FbxString*> details;
		FbxSceneCheckUtility sceneCheck(FbxCast<FbxScene>(mScene), &status, &details);
		bool lNotify = (!sceneCheck.Validate(FbxSceneCheckUtility::eCkeckData) && details.GetCount() > 0) || (mImporter->GetStatus().GetCode() != FbxStatus::eSuccess);
		if (lNotify)
		{
			FBXSDK_printf("\n");
			FBXSDK_printf("********************************************************************************\n");
			if (details.GetCount())
			{
				FBXSDK_printf("Scene integrity verification failed with the following errors:\n");

				for (int i = 0; i < details.GetCount(); i++)
					FBXSDK_printf("   %s\n", details[i]->Buffer());

				FbxArrayDelete<FbxString*>(details);
			}

			if (mImporter->GetStatus().GetCode() != FbxStatus::eSuccess)
			{
				FBXSDK_printf("\n");
				FBXSDK_printf("WARNING:\n");
				FBXSDK_printf("   The importer was able to read the file but with errors.\n");
				FBXSDK_printf("   Loaded scene may be incomplete.\n\n");
				FBXSDK_printf("   Last error message:'%s'\n", mImporter->GetStatus().GetErrorString());
			}

			FBXSDK_printf("********************************************************************************\n");
			FBXSDK_printf("\n");
		}

		// Set the scene status flag to refresh 
		// the scene in the first timer callback.

		// Convert Axis System to what is used in this example, if needed
		FbxAxisSystem SceneAxisSystem = mScene->GetGlobalSettings().GetAxisSystem();
		FbxAxisSystem OurAxisSystem(FbxAxisSystem::eYAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eRightHanded);
		if (SceneAxisSystem != OurAxisSystem)
		{
			OurAxisSystem.ConvertScene(mScene);
		}

		// Convert Unit System to what is used in this example, if needed
		FbxSystemUnit SceneSystemUnit = mScene->GetGlobalSettings().GetSystemUnit();
		if (SceneSystemUnit.GetScaleFactor() != 1.0)
		{
			//The unit in this example is centimeter.
			FbxSystemUnit::cm.ConvertScene(mScene);
		}

		// Get the list of all the animation stack.
		mScene->FillAnimStackNameArray(mAnimStackNameArray);

		// Get the list of all the cameras in the scene.
		//FillCameraArray(mScene, mCameraArray);

		// Convert mesh, NURBS and patch into triangle mesh
		FbxGeometryConverter lGeomConverter(mSdkManager);
		try {
			lGeomConverter.Triangulate(mScene, /*replace*/true);
		}
		catch (std::runtime_error) {
			FBXSDK_printf("Scene integrity verification failed.\n");
			return false;
		}

		// Bake the scene for one frame
		LoadCacheRecursive(mScene, mCurrentAnimLayer, filePath.c_str(), mSupportVBO);

		// Convert any .PC2 point cache data into the .MC format for 
		// vertex cache deformer playback.
		PreparePointCacheData(mScene, mCache_Start, mCache_Stop);

		// Get the list of pose in the scene
		FillPoseArray(mScene, mPoseArray);

	}
	else
	{
		printf("error: failed to import file: %s\n", mImporter->GetStatus().GetErrorString());
	}

	// Destroy the importer to release the file.
	mImporter->Destroy();
	mImporter = NULL;

	mSdkManager->Destroy();


	return lResult;
}


void FBXMesh::InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene)
{
    //The first thing to do is to create the FBX Manager which is the object allocator for almost all the classes in the SDK
    pManager = FbxManager::Create();
    if (!pManager)
    {
        FBXSDK_printf("Error: Unable to create FBX Manager!\n");
        exit(1);
    }
    else FBXSDK_printf("Autodesk FBX SDK version %s\n", pManager->GetVersion());

    //Create an IOSettings object. This object holds all import/export settings.
    FbxIOSettings* ios = FbxIOSettings::Create(pManager, IOSROOT);
    pManager->SetIOSettings(ios);

    //Load plugins from the executable directory (optional)
    FbxString lPath = FbxGetApplicationDirectory();
    pManager->LoadPluginsDirectory(lPath.Buffer());

    //Create an FBX scene. This object holds most objects imported/exported from/to files.
    pScene = FbxScene::Create(pManager, "My Scene");
    if (!pScene)
    {
        FBXSDK_printf("Error: Unable to create FBX scene!\n");
        exit(1);
    }
}

void FBXMesh::LoadCacheRecursive(FbxNode* pNode, FbxAnimLayer* pAnimLayer, bool pSupportVBO)
{
    // Bake material and hook as user data.
    const int lMaterialCount = pNode->GetMaterialCount();
    for (int lMaterialIndex = 0; lMaterialIndex < lMaterialCount; ++lMaterialIndex)
    {
        FbxSurfaceMaterial* lMaterial = pNode->GetMaterial(lMaterialIndex);
        if (lMaterial && !lMaterial->GetUserDataPtr())
        {
            FbxAutoPtr<MaterialCache> lMaterialCache(new MaterialCache);
            if (lMaterialCache->Initialize(lMaterial))
            {
                lMaterial->SetUserDataPtr(lMaterialCache.Release());
            }
        }
    }

    FbxNodeAttribute* lNodeAttribute = pNode->GetNodeAttribute();
    if (lNodeAttribute)
    {
        // Bake mesh as VBO(vertex buffer object) into GPU.
        if (lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh)
        {
            FbxMesh* lMesh = pNode->GetMesh();
            if (pSupportVBO && lMesh && !lMesh->GetUserDataPtr())
            {
                //FbxAutoPtr<VBOMesh> lMeshCache(new VBOMesh);
                //if (lMeshCache->Initialize(lMesh))
                //{
                //    lMesh->SetUserDataPtr(lMeshCache.Release());
                //}
            }
        }
        // Bake light properties.
        else if (lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eLight)
        {
            //FbxLight* lLight = pNode->GetLight();
            //if (lLight && !lLight->GetUserDataPtr())
            //{
            //    FbxAutoPtr<LightCache> lLightCache(new LightCache);
            //    if (lLightCache->Initialize(lLight, pAnimLayer))
            //    {
            //        lLight->SetUserDataPtr(lLightCache.Release());
            //    }
            //}
        }
    }

    const int lChildCount = pNode->GetChildCount();
    for (int lChildIndex = 0; lChildIndex < lChildCount; ++lChildIndex)
    {
        LoadCacheRecursive(pNode->GetChild(lChildIndex), pAnimLayer, pSupportVBO);
    }
}

void FBXMesh::LoadCacheRecursive(FbxScene* pScene, FbxAnimLayer* pAnimLayer, const char* pFbxFileName, bool pSupportVBO)
{
    // Load the textures into GPU, only for file texture now
    const int lTextureCount = pScene->GetTextureCount();
    for (int lTextureIndex = 0; lTextureIndex < lTextureCount; ++lTextureIndex)
    {
        FbxTexture* lTexture = pScene->GetTexture(lTextureIndex);
        FbxFileTexture* lFileTexture = FbxCast<FbxFileTexture>(lTexture);
        if (lFileTexture && !lFileTexture->GetUserDataPtr())
        {
            // Try to load the texture from absolute path
            const FbxString lFileName = lFileTexture->GetFileName();

            // Only TGA textures are supported now.
            if (lFileName.Right(3).Upper() != "TGA")
            {
                FBXSDK_printf("Only TGA textures are supported now: %s\n", lFileName.Buffer());
                continue;
            }

            GLuint lTextureObject = 0;
            bool lStatus = Texture::LoadTextureFromFile(lFileName.Buffer(), lTextureObject);

            const FbxString lAbsFbxFileName = FbxPathUtils::Resolve(pFbxFileName);
            const FbxString lAbsFolderName = FbxPathUtils::GetFolderName(lAbsFbxFileName);
            if (!lStatus)
            {
                // Load texture from relative file name (relative to FBX file)
                const FbxString lResolvedFileName = FbxPathUtils::Bind(lAbsFolderName, lFileTexture->GetRelativeFileName());
                lStatus = Texture::LoadTextureFromFile(lResolvedFileName.Buffer(), lTextureObject);
            }

            if (!lStatus)
            {
                // Load texture from file name only (relative to FBX file)
                const FbxString lTextureFileName = FbxPathUtils::GetFileName(lFileName);
                const FbxString lResolvedFileName = FbxPathUtils::Bind(lAbsFolderName, lTextureFileName);
                lStatus = Texture::LoadTextureFromFile(lResolvedFileName.Buffer(), lTextureObject);
            }

            if (!lStatus)
            {
                FBXSDK_printf("Failed to load texture file: %s\n", lFileName.Buffer());
                continue;
            }

            if (lStatus)
            {
                GLuint* lTextureName = new GLuint(lTextureObject);
                lFileTexture->SetUserDataPtr(lTextureName);
            }
        }
    }

    LoadCacheRecursive(pScene->GetRootNode(), pAnimLayer, pSupportVBO);
}


void FBXMesh::PreparePointCacheData(FbxScene* pScene, FbxTime& pCache_Start, FbxTime& pCache_Stop)
{
    // This function show how to cycle through scene elements in a linear way.
    const int lNodeCount = pScene->GetSrcObjectCount<FbxNode>();
    FbxStatus lStatus;

    for (int lIndex = 0; lIndex < lNodeCount; lIndex++)
    {
        FbxNode* lNode = pScene->GetSrcObject<FbxNode>(lIndex);

        if (lNode->GetGeometry())
        {
            int i, lVertexCacheDeformerCount = lNode->GetGeometry()->GetDeformerCount(FbxDeformer::eVertexCache);

            // There should be a maximum of 1 Vertex Cache Deformer for the moment
            lVertexCacheDeformerCount = lVertexCacheDeformerCount > 0 ? 1 : 0;

            for (i = 0; i < lVertexCacheDeformerCount; ++i)
            {
                // Get the Point Cache object
                FbxVertexCacheDeformer* lDeformer = static_cast<FbxVertexCacheDeformer*>(lNode->GetGeometry()->GetDeformer(i, FbxDeformer::eVertexCache));
                if (!lDeformer) continue;
                FbxCache* lCache = lDeformer->GetCache();
                if (!lCache) continue;

                // Process the point cache data only if the constraint is active
                if (lDeformer->Active.Get())
                {
                    if (lCache->GetCacheFileFormat() == FbxCache::eMaxPointCacheV2)
                    {
                        // This code show how to convert from PC2 to MC point cache format
                        // turn it on if you need it.
#if 0 
                        if (!lCache->ConvertFromPC2ToMC(FbxCache::eMCOneFile,
                            FbxTime::GetFrameRate(pScene->GetGlobalTimeSettings().GetTimeMode())))
                        {
                            // Conversion failed, retrieve the error here
                            FbxString lTheErrorIs = lCache->GetStaus().GetErrorString();
                        }
#endif
                    }
                    else if (lCache->GetCacheFileFormat() == FbxCache::eMayaCache)
                    {
                        // This code show how to convert from MC to PC2 point cache format
                        // turn it on if you need it.
                        //#if 0 
                        if (!lCache->ConvertFromMCToPC2(FbxTime::GetFrameRate(pScene->GetGlobalSettings().GetTimeMode()), 0, &lStatus))
                        {
                            // Conversion failed, retrieve the error here
                            FbxString lTheErrorIs = lStatus.GetErrorString();
                        }
                        //#endif
                    }


                    // Now open the cache file to read from it
                    if (!lCache->OpenFileForRead(&lStatus))
                    {
                        // Cannot open file 
                        FbxString lTheErrorIs = lStatus.GetErrorString();

                        // Set the deformer inactive so we don't play it back
                        lDeformer->Active = false;
                    }
                    else
                    {
                        // get the start and stop time of the cache
                        FbxTime lChannel_Start;
                        FbxTime lChannel_Stop;
                        int lChannelIndex = lCache->GetChannelIndex(lDeformer->Channel.Get());
                        if (lCache->GetAnimationRange(lChannelIndex, lChannel_Start, lChannel_Stop))
                        {
                            // get the smallest start time
                            if (lChannel_Start < pCache_Start) pCache_Start = lChannel_Start;

                            // get the biggest stop time
                            if (lChannel_Stop > pCache_Stop)  pCache_Stop = lChannel_Stop;
                        }
                    }
                }
            }
        }
    }
}

void FBXMesh::FillPoseArray(FbxScene* pScene, FbxArray<FbxPose*>& pPoseArray)
{
    const int lPoseCount = pScene->GetPoseCount();

    for (int i = 0; i < lPoseCount; ++i)
    {
        pPoseArray.Add(pScene->GetPose(i));
    }
}

void FBXMesh::Draw(Shader* shader)
{
    glPushAttrib(GL_ENABLE_BIT);
    glPushAttrib(GL_LIGHTING_BIT);
    glEnable(GL_DEPTH_TEST);
    // Draw the front face only, except for the texts and lights.
    glEnable(GL_CULL_FACE);

    // Set the view to the current camera settings.
    //SetCamera(mScene, mCurrentTime, mCurrentAnimLayer, mCameraArray,
    //    mWindowWidth, mWindowHeight);

    FbxPose* lPose = NULL;
    //if (mPoseIndex != -1)
    //{
    //    lPose = mScene->GetPose(mPoseIndex);
    //}

    // If one node is selected, draw it and its children.
    FbxAMatrix lDummyGlobalPosition;

    //if (mSelectedNode)
    //{
    //    // Set the lighting before other things.
    //    InitializeLights(mScene, mCurrentTime, lPose);
    //    DrawNodeRecursive(mSelectedNode, mCurrentTime, mCurrentAnimLayer, lDummyGlobalPosition, lPose, mShadingMode);
    //    DisplayGrid(lDummyGlobalPosition);
    //}
    //// Otherwise, draw the whole scene.
    //else
    //{
    //}
    //InitializeLights(mScene, mCurrentTime, lPose);
    DrawNodeRecursive(mScene->GetRootNode(), mCurrentAnimLayer, lDummyGlobalPosition, lPose);
    //DisplayGrid(lDummyGlobalPosition);

    glPopAttrib();
    glPopAttrib();
}

void FBXMesh::DrawNodeRecursive(FbxNode* pNode, FbxAnimLayer* pAnimLayer,
    FbxAMatrix& pParentGlobalPosition, FbxPose* pPose)
{
    //FbxAMatrix lGlobalPosition = GetGlobalPosition(pNode, pTime, pPose, &pParentGlobalPosition);

    if (pNode->GetNodeAttribute())
    {
        // Geometry offset.
        // it is not inherited by the children.
        //FbxAMatrix lGeometryOffset = GetGeometry(pNode);
        //FbxAMatrix lGlobalOffPosition = lGlobalPosition * lGeometryOffset;
        FbxAMatrix lGeometryOffset;
        FbxAMatrix lGlobalOffPosition;
        lGeometryOffset.SetIdentity();
        lGlobalOffPosition.SetIdentity();
        DrawNode(pNode, pAnimLayer, pParentGlobalPosition, lGlobalOffPosition, pPose);
    }

    const int lChildCount = pNode->GetChildCount();
    for (int lChildIndex = 0; lChildIndex < lChildCount; ++lChildIndex)
    {
        FbxAMatrix globalPos; globalPos.SetIdentity();
        DrawNodeRecursive(pNode->GetChild(lChildIndex), pAnimLayer, globalPos, pPose);
    }
}

void FBXMesh::DrawNode(FbxNode* pNode,
    FbxAnimLayer* pAnimLayer,
    FbxAMatrix& pParentGlobalPosition,
    FbxAMatrix& pGlobalPosition,
    FbxPose* pPose)
{
    FbxNodeAttribute* lNodeAttribute = pNode->GetNodeAttribute();

    if (lNodeAttribute)
    {
        // All lights has been processed before the whole scene because they influence every geometry.
        if (lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eMarker)
        {
            //DrawMarker(pGlobalPosition);
        }
        else if (lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eSkeleton)
        {
            //DrawSkeleton(pNode, pParentGlobalPosition, pGlobalPosition);
        }
        // NURBS and patch have been converted into triangluation meshes.
        else if (lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh)
        {
            DrawMesh(pNode, pAnimLayer, pGlobalPosition, pPose);
        }
        //    else if (lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eCamera)
        //    {
        //        //DrawCamera(pNode, pTime, pAnimLayer, pGlobalPosition);
        //    }
        //    else if (lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eNull)
        //    {
        //        DrawNull(pGlobalPosition);
        //    }
        //}
        //else
        //{
        //    // Draw a Null for nodes without attribute.
        //    DrawNull(pGlobalPosition);
        //}
    }
}

void FBXMesh::DrawMesh(FbxNode* pNode, FbxAnimLayer* pAnimLayer,
    FbxAMatrix& pGlobalPosition, FbxPose* pPose)
{
    FbxMesh* lMesh = pNode->GetMesh();
    const int lVertexCount = lMesh->GetControlPointsCount();

    // No vertex to draw.
    if (lVertexCount == 0)
    {
        return;
    }

    const VBOMesh* lMeshCache = static_cast<const VBOMesh*>(lMesh->GetUserDataPtr());

    // If it has some defomer connection, update the vertices position
    const bool lHasVertexCache = lMesh->GetDeformerCount(FbxDeformer::eVertexCache) &&
        (static_cast<FbxVertexCacheDeformer*>(lMesh->GetDeformer(0, FbxDeformer::eVertexCache)))->Active.Get();
    const bool lHasShape = lMesh->GetShapeCount() > 0;
    const bool lHasSkin = lMesh->GetDeformerCount(FbxDeformer::eSkin) > 0;
    const bool lHasDeformation = lHasVertexCache || lHasShape || lHasSkin;

    FbxVector4* lVertexArray = NULL;
    if (!lMeshCache || lHasDeformation)
    {
        lVertexArray = new FbxVector4[lVertexCount];
        memcpy(lVertexArray, lMesh->GetControlPoints(), lVertexCount * sizeof(FbxVector4));
    }

    if (lHasDeformation)
    {
        // Active vertex cache deformer will overwrite any other deformer
        if (lHasVertexCache)
        {
            FbxTime time = 0;
            ReadVertexCacheData(lMesh, time, lVertexArray);
        }
        else
        {
            //if (lHasShape)
            //{
            //    // Deform the vertex array with the shapes.
            //    ComputeShapeDeformation(lMesh, pTime, pAnimLayer, lVertexArray);
            //}

            ////we need to get the number of clusters
            //const int lSkinCount = lMesh->GetDeformerCount(FbxDeformer::eSkin);
            //int lClusterCount = 0;
            //for (int lSkinIndex = 0; lSkinIndex < lSkinCount; ++lSkinIndex)
            //{
            //    lClusterCount += ((FbxSkin*)(lMesh->GetDeformer(lSkinIndex, FbxDeformer::eSkin)))->GetClusterCount();
            //}
            //if (lClusterCount)
            //{
            //    // Deform the vertex array with the skin deformer.
            //    ComputeSkinDeformation(pGlobalPosition, lMesh, pTime, lVertexArray, pPose);
            //}
        }

        if (lMeshCache)
            lMeshCache->UpdateVertexPosition(lMesh, lVertexArray);
    }

    glPushMatrix();
    glMultMatrixd((const double*)pGlobalPosition);

    lMeshCache->BeginDraw();
    const int lSubMeshCount = lMeshCache->GetSubMeshCount();
    for (int lIndex = 0; lIndex < lSubMeshCount; ++lIndex)
    {
        const FbxSurfaceMaterial* lMaterial = pNode->GetMaterial(lIndex);
        if (lMaterial)
        {
            const MaterialCache* lMaterialCache = static_cast<const MaterialCache*>(lMaterial->GetUserDataPtr());
            if (lMaterialCache)
            {
                lMaterialCache->SetCurrentMaterial();
            }
        }
        else
        {
            // Draw green for faces without material
            MaterialCache::SetDefaultMaterial();
        }

        lMeshCache->Draw(lIndex);
    }
    lMeshCache->EndDraw();

    glPopMatrix();

    delete[] lVertexArray;
}


void FBXMesh::ReadVertexCacheData(FbxMesh* pMesh,
    FbxTime& pTime,
    FbxVector4* pVertexArray)
{
    FbxVertexCacheDeformer* lDeformer = static_cast<FbxVertexCacheDeformer*>(pMesh->GetDeformer(0, FbxDeformer::eVertexCache));
    FbxCache* lCache = lDeformer->GetCache();
    int                     lChannelIndex = lCache->GetChannelIndex(lDeformer->Channel.Get());
    unsigned int            lVertexCount = (unsigned int)pMesh->GetControlPointsCount();
    bool                    lReadSucceed = false;
    float* lReadBuf = NULL;
    unsigned int			BufferSize = 0;

    if (lDeformer->Type.Get() != FbxVertexCacheDeformer::ePositions)
        // only process positions
        return;

    unsigned int Length = 0;
    lCache->Read(NULL, Length, FBXSDK_TIME_ZERO, lChannelIndex);
    if (Length != lVertexCount * 3)
        // the content of the cache is by vertex not by control points (we don't support it here)
        return;

    lReadSucceed = lCache->Read(&lReadBuf, BufferSize, pTime, lChannelIndex);
    if (lReadSucceed)
    {
        unsigned int lReadBufIndex = 0;

        while (lReadBufIndex < 3 * lVertexCount)
        {
            // In statements like "pVertexArray[lReadBufIndex/3].SetAt(2, lReadBuf[lReadBufIndex++])", 
            // on Mac platform, "lReadBufIndex++" is evaluated before "lReadBufIndex/3". 
            // So separate them.
            pVertexArray[lReadBufIndex / 3].mData[0] = lReadBuf[lReadBufIndex]; lReadBufIndex++;
            pVertexArray[lReadBufIndex / 3].mData[1] = lReadBuf[lReadBufIndex]; lReadBufIndex++;
            pVertexArray[lReadBufIndex / 3].mData[2] = lReadBuf[lReadBufIndex]; lReadBufIndex++;
        }
    }
}

//
//void FBXMesh::ComputeShapeDeformation(FbxMesh* pMesh, FbxTime& pTime, FbxAnimLayer* pAnimLayer, FbxVector4* pVertexArray)
//{
//    int lVertexCount = pMesh->GetControlPointsCount();
//
//    FbxVector4* lSrcVertexArray = pVertexArray;
//    FbxVector4* lDstVertexArray = new FbxVector4[lVertexCount];
//    memcpy(lDstVertexArray, pVertexArray, lVertexCount * sizeof(FbxVector4));
//
//    int lBlendShapeDeformerCount = pMesh->GetDeformerCount(FbxDeformer::eBlendShape);
//    for (int lBlendShapeIndex = 0; lBlendShapeIndex < lBlendShapeDeformerCount; ++lBlendShapeIndex)
//    {
//        FbxBlendShape* lBlendShape = (FbxBlendShape*)pMesh->GetDeformer(lBlendShapeIndex, FbxDeformer::eBlendShape);
//
//        int lBlendShapeChannelCount = lBlendShape->GetBlendShapeChannelCount();
//        for (int lChannelIndex = 0; lChannelIndex < lBlendShapeChannelCount; ++lChannelIndex)
//        {
//            FbxBlendShapeChannel* lChannel = lBlendShape->GetBlendShapeChannel(lChannelIndex);
//            if (lChannel)
//            {
//                // Get the percentage of influence on this channel.
//                FbxAnimCurve* lFCurve = pMesh->GetShapeChannel(lBlendShapeIndex, lChannelIndex, pAnimLayer);
//                if (!lFCurve) continue;
//                double lWeight = lFCurve->Evaluate(pTime);
//
//                /*
//                If there is only one targetShape on this channel, the influence is easy to calculate:
//                influence = (targetShape - baseGeometry) * weight * 0.01
//                dstGeometry = baseGeometry + influence
//
//                But if there are more than one targetShapes on this channel, this is an in-between
//                blendshape, also called progressive morph. The calculation of influence is different.
//
//                For example, given two in-between targets, the full weight percentage of first target
//                is 50, and the full weight percentage of the second target is 100.
//                When the weight percentage reach 50, the base geometry is already be fully morphed
//                to the first target shape. When the weight go over 50, it begin to morph from the
//                first target shape to the second target shape.
//
//                To calculate influence when the weight percentage is 25:
//                1. 25 falls in the scope of 0 and 50, the morphing is from base geometry to the first target.
//                2. And since 25 is already half way between 0 and 50, so the real weight percentage change to
//                the first target is 50.
//                influence = (firstTargetShape - baseGeometry) * (25-0)/(50-0) * 100
//                dstGeometry = baseGeometry + influence
//
//                To calculate influence when the weight percentage is 75:
//                1. 75 falls in the scope of 50 and 100, the morphing is from the first target to the second.
//                2. And since 75 is already half way between 50 and 100, so the real weight percentage change
//                to the second target is 50.
//                influence = (secondTargetShape - firstTargetShape) * (75-50)/(100-50) * 100
//                dstGeometry = firstTargetShape + influence
//                */
//
//                // Find the two shape indices for influence calculation according to the weight.
//                // Consider index of base geometry as -1.
//
//                int lShapeCount = lChannel->GetTargetShapeCount();
//                double* lFullWeights = lChannel->GetTargetShapeFullWeights();
//
//                // Find out which scope the lWeight falls in.
//                int lStartIndex = -1;
//                int lEndIndex = -1;
//                for (int lShapeIndex = 0; lShapeIndex < lShapeCount; ++lShapeIndex)
//                {
//                    if (lWeight > 0 && lWeight <= lFullWeights[0])
//                    {
//                        lEndIndex = 0;
//                        break;
//                    }
//                    if (lWeight > lFullWeights[lShapeIndex] && lWeight < lFullWeights[lShapeIndex + 1])
//                    {
//                        lStartIndex = lShapeIndex;
//                        lEndIndex = lShapeIndex + 1;
//                        break;
//                    }
//                }
//
//                FbxShape* lStartShape = NULL;
//                FbxShape* lEndShape = NULL;
//                if (lStartIndex > -1)
//                {
//                    lStartShape = lChannel->GetTargetShape(lStartIndex);
//                }
//                if (lEndIndex > -1)
//                {
//                    lEndShape = lChannel->GetTargetShape(lEndIndex);
//                }
//
//                //The weight percentage falls between base geometry and the first target shape.
//                if (lStartIndex == -1 && lEndShape)
//                {
//                    double lEndWeight = lFullWeights[0];
//                    // Calculate the real weight.
//                    lWeight = (lWeight / lEndWeight) * 100;
//                    // Initialize the lDstVertexArray with vertex of base geometry.
//                    memcpy(lDstVertexArray, lSrcVertexArray, lVertexCount * sizeof(FbxVector4));
//                    for (int j = 0; j < lVertexCount; j++)
//                    {
//                        // Add the influence of the shape vertex to the mesh vertex.
//                        FbxVector4 lInfluence = (lEndShape->GetControlPoints()[j] - lSrcVertexArray[j]) * lWeight * 0.01;
//                        lDstVertexArray[j] += lInfluence;
//                    }
//                }
//                //The weight percentage falls between two target shapes.
//                else if (lStartShape && lEndShape)
//                {
//                    double lStartWeight = lFullWeights[lStartIndex];
//                    double lEndWeight = lFullWeights[lEndIndex];
//                    // Calculate the real weight.
//                    lWeight = ((lWeight - lStartWeight) / (lEndWeight - lStartWeight)) * 100;
//                    // Initialize the lDstVertexArray with vertex of the previous target shape geometry.
//                    memcpy(lDstVertexArray, lStartShape->GetControlPoints(), lVertexCount * sizeof(FbxVector4));
//                    for (int j = 0; j < lVertexCount; j++)
//                    {
//                        // Add the influence of the shape vertex to the previous shape vertex.
//                        FbxVector4 lInfluence = (lEndShape->GetControlPoints()[j] - lStartShape->GetControlPoints()[j]) * lWeight * 0.01;
//                        lDstVertexArray[j] += lInfluence;
//                    }
//                }
//            }//If lChannel is valid
//        }//For each blend shape channel
//    }//For each blend shape deformer
//
//    memcpy(pVertexArray, lDstVertexArray, lVertexCount * sizeof(FbxVector4));
//
//    delete[] lDstVertexArray;
//}
//
//
////Compute the transform matrix that the cluster will transform the vertex.
//void ComputeClusterDeformation(FbxAMatrix& pGlobalPosition,
//    FbxMesh* pMesh,
//    FbxCluster* pCluster,
//    FbxAMatrix& pVertexTransformMatrix,
//    FbxTime pTime,
//    FbxPose* pPose)
//{
//    FbxCluster::ELinkMode lClusterMode = pCluster->GetLinkMode();
//
//    FbxAMatrix lReferenceGlobalInitPosition;
//    FbxAMatrix lReferenceGlobalCurrentPosition;
//    FbxAMatrix lAssociateGlobalInitPosition;
//    FbxAMatrix lAssociateGlobalCurrentPosition;
//    FbxAMatrix lClusterGlobalInitPosition;
//    FbxAMatrix lClusterGlobalCurrentPosition;
//
//    FbxAMatrix lReferenceGeometry;
//    FbxAMatrix lAssociateGeometry;
//    FbxAMatrix lClusterGeometry;
//
//    FbxAMatrix lClusterRelativeInitPosition;
//    FbxAMatrix lClusterRelativeCurrentPositionInverse;
//
//    if (lClusterMode == FbxCluster::eAdditive && pCluster->GetAssociateModel())
//    {
//        pCluster->GetTransformAssociateModelMatrix(lAssociateGlobalInitPosition);
//        // Geometric transform of the model
//        lAssociateGeometry = GetGeometry(pCluster->GetAssociateModel());
//        lAssociateGlobalInitPosition *= lAssociateGeometry;
//        lAssociateGlobalCurrentPosition = GetGlobalPosition(pCluster->GetAssociateModel(), pTime, pPose);
//
//        pCluster->GetTransformMatrix(lReferenceGlobalInitPosition);
//        // Multiply lReferenceGlobalInitPosition by Geometric Transformation
//        lReferenceGeometry = GetGeometry(pMesh->GetNode());
//        lReferenceGlobalInitPosition *= lReferenceGeometry;
//        lReferenceGlobalCurrentPosition = pGlobalPosition;
//
//        // Get the link initial global position and the link current global position.
//        pCluster->GetTransformLinkMatrix(lClusterGlobalInitPosition);
//        // Multiply lClusterGlobalInitPosition by Geometric Transformation
//        lClusterGeometry = GetGeometry(pCluster->GetLink());
//        lClusterGlobalInitPosition *= lClusterGeometry;
//        lClusterGlobalCurrentPosition = GetGlobalPosition(pCluster->GetLink(), pTime, pPose);
//
//        // Compute the shift of the link relative to the reference.
//        //ModelM-1 * AssoM * AssoGX-1 * LinkGX * LinkM-1*ModelM
//        pVertexTransformMatrix = lReferenceGlobalInitPosition.Inverse() * lAssociateGlobalInitPosition * lAssociateGlobalCurrentPosition.Inverse() *
//            lClusterGlobalCurrentPosition * lClusterGlobalInitPosition.Inverse() * lReferenceGlobalInitPosition;
//    }
//    else
//    {
//        pCluster->GetTransformMatrix(lReferenceGlobalInitPosition);
//        lReferenceGlobalCurrentPosition = pGlobalPosition;
//        // Multiply lReferenceGlobalInitPosition by Geometric Transformation
//        lReferenceGeometry = GetGeometry(pMesh->GetNode());
//        lReferenceGlobalInitPosition *= lReferenceGeometry;
//
//        // Get the link initial global position and the link current global position.
//        pCluster->GetTransformLinkMatrix(lClusterGlobalInitPosition);
//        lClusterGlobalCurrentPosition = GetGlobalPosition(pCluster->GetLink(), pTime, pPose);
//
//        // Compute the initial position of the link relative to the reference.
//        lClusterRelativeInitPosition = lClusterGlobalInitPosition.Inverse() * lReferenceGlobalInitPosition;
//
//        // Compute the current position of the link relative to the reference.
//        lClusterRelativeCurrentPositionInverse = lReferenceGlobalCurrentPosition.Inverse() * lClusterGlobalCurrentPosition;
//
//        // Compute the shift of the link relative to the reference.
//        pVertexTransformMatrix = lClusterRelativeCurrentPositionInverse * lClusterRelativeInitPosition;
//    }
//}
//
//// Deform the vertex array in classic linear way.
//void ComputeLinearDeformation(FbxAMatrix& pGlobalPosition,
//    FbxMesh* pMesh,
//    FbxTime& pTime,
//    FbxVector4* pVertexArray,
//    FbxPose* pPose)
//{
//    // All the links must have the same link mode.
//    FbxCluster::ELinkMode lClusterMode = ((FbxSkin*)pMesh->GetDeformer(0, FbxDeformer::eSkin))->GetCluster(0)->GetLinkMode();
//
//    int lVertexCount = pMesh->GetControlPointsCount();
//    FbxAMatrix* lClusterDeformation = new FbxAMatrix[lVertexCount];
//    memset(lClusterDeformation, 0, lVertexCount * (unsigned int)sizeof(FbxAMatrix));
//
//    double* lClusterWeight = new double[lVertexCount];
//    memset(lClusterWeight, 0, lVertexCount * sizeof(double));
//
//    if (lClusterMode == FbxCluster::eAdditive)
//    {
//        for (int i = 0; i < lVertexCount; ++i)
//        {
//            lClusterDeformation[i].SetIdentity();
//        }
//    }
//
//    // For all skins and all clusters, accumulate their deformation and weight
//    // on each vertices and store them in lClusterDeformation and lClusterWeight.
//    int lSkinCount = pMesh->GetDeformerCount(FbxDeformer::eSkin);
//    for (int lSkinIndex = 0; lSkinIndex < lSkinCount; ++lSkinIndex)
//    {
//        FbxSkin* lSkinDeformer = (FbxSkin*)pMesh->GetDeformer(lSkinIndex, FbxDeformer::eSkin);
//
//        int lClusterCount = lSkinDeformer->GetClusterCount();
//        for (int lClusterIndex = 0; lClusterIndex < lClusterCount; ++lClusterIndex)
//        {
//            FbxCluster* lCluster = lSkinDeformer->GetCluster(lClusterIndex);
//            if (!lCluster->GetLink())
//                continue;
//
//            FbxAMatrix lVertexTransformMatrix;
//            ComputeClusterDeformation(pGlobalPosition, pMesh, lCluster, lVertexTransformMatrix, pTime, pPose);
//
//            int lVertexIndexCount = lCluster->GetControlPointIndicesCount();
//            for (int k = 0; k < lVertexIndexCount; ++k)
//            {
//                int lIndex = lCluster->GetControlPointIndices()[k];
//
//                // Sometimes, the mesh can have less points than at the time of the skinning
//                // because a smooth operator was active when skinning but has been deactivated during export.
//                if (lIndex >= lVertexCount)
//                    continue;
//
//                double lWeight = lCluster->GetControlPointWeights()[k];
//
//                if (lWeight == 0.0)
//                {
//                    continue;
//                }
//
//                // Compute the influence of the link on the vertex.
//                FbxAMatrix lInfluence = lVertexTransformMatrix;
//                MatrixScale(lInfluence, lWeight);
//
//                if (lClusterMode == FbxCluster::eAdditive)
//                {
//                    // Multiply with the product of the deformations on the vertex.
//                    MatrixAddToDiagonal(lInfluence, 1.0 - lWeight);
//                    lClusterDeformation[lIndex] = lInfluence * lClusterDeformation[lIndex];
//
//                    // Set the link to 1.0 just to know this vertex is influenced by a link.
//                    lClusterWeight[lIndex] = 1.0;
//                }
//                else // lLinkMode == FbxCluster::eNormalize || lLinkMode == FbxCluster::eTotalOne
//                {
//                    // Add to the sum of the deformations on the vertex.
//                    MatrixAdd(lClusterDeformation[lIndex], lInfluence);
//
//                    // Add to the sum of weights to either normalize or complete the vertex.
//                    lClusterWeight[lIndex] += lWeight;
//                }
//            }//For each vertex			
//        }//lClusterCount
//    }
//
//    //Actually deform each vertices here by information stored in lClusterDeformation and lClusterWeight
//    for (int i = 0; i < lVertexCount; i++)
//    {
//        FbxVector4 lSrcVertex = pVertexArray[i];
//        FbxVector4& lDstVertex = pVertexArray[i];
//        double lWeight = lClusterWeight[i];
//
//        // Deform the vertex if there was at least a link with an influence on the vertex,
//        if (lWeight != 0.0)
//        {
//            lDstVertex = lClusterDeformation[i].MultT(lSrcVertex);
//            if (lClusterMode == FbxCluster::eNormalize)
//            {
//                // In the normalized link mode, a vertex is always totally influenced by the links. 
//                lDstVertex /= lWeight;
//            }
//            else if (lClusterMode == FbxCluster::eTotalOne)
//            {
//                // In the total 1 link mode, a vertex can be partially influenced by the links. 
//                lSrcVertex *= (1.0 - lWeight);
//                lDstVertex += lSrcVertex;
//            }
//        }
//    }
//
//    delete[] lClusterDeformation;
//    delete[] lClusterWeight;
//}
//
//// Deform the vertex array in Dual Quaternion Skinning way.
//void ComputeDualQuaternionDeformation(FbxAMatrix& pGlobalPosition,
//    FbxMesh* pMesh,
//    FbxTime& pTime,
//    FbxVector4* pVertexArray,
//    FbxPose* pPose)
//{
//    // All the links must have the same link mode.
//    FbxCluster::ELinkMode lClusterMode = ((FbxSkin*)pMesh->GetDeformer(0, FbxDeformer::eSkin))->GetCluster(0)->GetLinkMode();
//
//    int lVertexCount = pMesh->GetControlPointsCount();
//    int lSkinCount = pMesh->GetDeformerCount(FbxDeformer::eSkin);
//
//    FbxDualQuaternion* lDQClusterDeformation = new FbxDualQuaternion[lVertexCount];
//    memset(lDQClusterDeformation, 0, lVertexCount * (unsigned int)sizeof(FbxDualQuaternion));
//
//    double* lClusterWeight = new double[lVertexCount];
//    memset(lClusterWeight, 0, lVertexCount * (unsigned int)sizeof(double));
//
//    // For all skins and all clusters, accumulate their deformation and weight
//    // on each vertices and store them in lClusterDeformation and lClusterWeight.
//    for (int lSkinIndex = 0; lSkinIndex < lSkinCount; ++lSkinIndex)
//    {
//        FbxSkin* lSkinDeformer = (FbxSkin*)pMesh->GetDeformer(lSkinIndex, FbxDeformer::eSkin);
//        int lClusterCount = lSkinDeformer->GetClusterCount();
//        for (int lClusterIndex = 0; lClusterIndex < lClusterCount; ++lClusterIndex)
//        {
//            FbxCluster* lCluster = lSkinDeformer->GetCluster(lClusterIndex);
//            if (!lCluster->GetLink())
//                continue;
//
//            FbxAMatrix lVertexTransformMatrix;
//            ComputeClusterDeformation(pGlobalPosition, pMesh, lCluster, lVertexTransformMatrix, pTime, pPose);
//
//            FbxQuaternion lQ = lVertexTransformMatrix.GetQ();
//            FbxVector4 lT = lVertexTransformMatrix.GetT();
//            FbxDualQuaternion lDualQuaternion(lQ, lT);
//
//            int lVertexIndexCount = lCluster->GetControlPointIndicesCount();
//            for (int k = 0; k < lVertexIndexCount; ++k)
//            {
//                int lIndex = lCluster->GetControlPointIndices()[k];
//
//                // Sometimes, the mesh can have less points than at the time of the skinning
//                // because a smooth operator was active when skinning but has been deactivated during export.
//                if (lIndex >= lVertexCount)
//                    continue;
//
//                double lWeight = lCluster->GetControlPointWeights()[k];
//
//                if (lWeight == 0.0)
//                    continue;
//
//                // Compute the influence of the link on the vertex.
//                FbxDualQuaternion lInfluence = lDualQuaternion * lWeight;
//                if (lClusterMode == FbxCluster::eAdditive)
//                {
//                    // Simply influenced by the dual quaternion.
//                    lDQClusterDeformation[lIndex] = lInfluence;
//
//                    // Set the link to 1.0 just to know this vertex is influenced by a link.
//                    lClusterWeight[lIndex] = 1.0;
//                }
//                else // lLinkMode == FbxCluster::eNormalize || lLinkMode == FbxCluster::eTotalOne
//                {
//                    if (lClusterIndex == 0)
//                    {
//                        lDQClusterDeformation[lIndex] = lInfluence;
//                    }
//                    else
//                    {
//                        // Add to the sum of the deformations on the vertex.
//                        // Make sure the deformation is accumulated in the same rotation direction. 
//                        // Use dot product to judge the sign.
//                        double lSign = lDQClusterDeformation[lIndex].GetFirstQuaternion().DotProduct(lDualQuaternion.GetFirstQuaternion());
//                        if (lSign >= 0.0)
//                        {
//                            lDQClusterDeformation[lIndex] += lInfluence;
//                        }
//                        else
//                        {
//                            lDQClusterDeformation[lIndex] -= lInfluence;
//                        }
//                    }
//                    // Add to the sum of weights to either normalize or complete the vertex.
//                    lClusterWeight[lIndex] += lWeight;
//                }
//            }//For each vertex
//        }//lClusterCount
//    }
//
//    //Actually deform each vertices here by information stored in lClusterDeformation and lClusterWeight
//    for (int i = 0; i < lVertexCount; i++)
//    {
//        FbxVector4 lSrcVertex = pVertexArray[i];
//        FbxVector4& lDstVertex = pVertexArray[i];
//        double lWeightSum = lClusterWeight[i];
//
//        // Deform the vertex if there was at least a link with an influence on the vertex,
//        if (lWeightSum != 0.0)
//        {
//            lDQClusterDeformation[i].Normalize();
//            lDstVertex = lDQClusterDeformation[i].Deform(lDstVertex);
//
//            if (lClusterMode == FbxCluster::eNormalize)
//            {
//                // In the normalized link mode, a vertex is always totally influenced by the links. 
//                lDstVertex /= lWeightSum;
//            }
//            else if (lClusterMode == FbxCluster::eTotalOne)
//            {
//                // In the total 1 link mode, a vertex can be partially influenced by the links. 
//                lSrcVertex *= (1.0 - lWeightSum);
//                lDstVertex += lSrcVertex;
//            }
//        }
//    }
//
//    delete[] lDQClusterDeformation;
//    delete[] lClusterWeight;
//}
//
//// Deform the vertex array according to the links contained in the mesh and the skinning type.
//void FBXMesh::ComputeSkinDeformation(FbxAMatrix& pGlobalPosition,
//    FbxMesh* pMesh,
//    FbxTime& pTime,
//    FbxVector4* pVertexArray,
//    FbxPose* pPose)
//{
//    FbxSkin* lSkinDeformer = (FbxSkin*)pMesh->GetDeformer(0, FbxDeformer::eSkin);
//    FbxSkin::EType lSkinningType = lSkinDeformer->GetSkinningType();
//
//    if (lSkinningType == FbxSkin::eLinear || lSkinningType == FbxSkin::eRigid)
//    {
//        ComputeLinearDeformation(pGlobalPosition, pMesh, pTime, pVertexArray, pPose);
//    }
//    else if (lSkinningType == FbxSkin::eDualQuaternion)
//    {
//        ComputeDualQuaternionDeformation(pGlobalPosition, pMesh, pTime, pVertexArray, pPose);
//    }
//    else if (lSkinningType == FbxSkin::eBlend)
//    {
//        int lVertexCount = pMesh->GetControlPointsCount();
//
//        FbxVector4* lVertexArrayLinear = new FbxVector4[lVertexCount];
//        memcpy(lVertexArrayLinear, pMesh->GetControlPoints(), lVertexCount * sizeof(FbxVector4));
//
//        FbxVector4* lVertexArrayDQ = new FbxVector4[lVertexCount];
//        memcpy(lVertexArrayDQ, pMesh->GetControlPoints(), lVertexCount * sizeof(FbxVector4));
//
//        ComputeLinearDeformation(pGlobalPosition, pMesh, pTime, lVertexArrayLinear, pPose);
//        ComputeDualQuaternionDeformation(pGlobalPosition, pMesh, pTime, lVertexArrayDQ, pPose);
//
//        // To blend the skinning according to the blend weights
//        // Final vertex = DQSVertex * blend weight + LinearVertex * (1- blend weight)
//        // DQSVertex: vertex that is deformed by dual quaternion skinning method;
//        // LinearVertex: vertex that is deformed by classic linear skinning method;
//        int lBlendWeightsCount = lSkinDeformer->GetControlPointIndicesCount();
//        for (int lBWIndex = 0; lBWIndex < lBlendWeightsCount; ++lBWIndex)
//        {
//            double lBlendWeight = lSkinDeformer->GetControlPointBlendWeights()[lBWIndex];
//            pVertexArray[lBWIndex] = lVertexArrayDQ[lBWIndex] * lBlendWeight + lVertexArrayLinear[lBWIndex] * (1 - lBlendWeight);
//        }
//    }
//}