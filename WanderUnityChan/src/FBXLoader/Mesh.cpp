#include "FBXLoader/Mesh.hpp"
#include <fbxsdk.h>
#include "glew.h"
#include "Texture.hpp"
#include "FBXLoader/MaterialCache.hpp"
#include "Shader.hpp"


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
    DrawNodeRecursive(mScene->GetRootNode(), mCurrentTime, mCurrentAnimLayer, lDummyGlobalPosition, lPose, mShadingMode);
    //DisplayGrid(lDummyGlobalPosition);

    glPopAttrib();
    glPopAttrib();
}